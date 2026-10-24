#include "../include/cpu.h"

extern int __vmx_vmread(uint64_t field, uint64_t *value);
extern int __vmx_vmwrite(uint64_t field, uint64_t value);
extern int __vmx_on(void *vmxon_phys);
extern int __vmx_off(void);
extern int __vmx_vmclear(void *vmcs_phys);
extern int __vmx_vmptrld(void *vmcs_phys);

void cpu_init_context(cpu_context_t *ctx) {
    if (!ctx) return;
    for (uint32_t i = 0; i < CPU_MAX_CORES; i++) {
        ctx->cores[i].cpu_id = i;
        ctx->cores[i].vmx_active = 0;
        ctx->cores[i].vmxon_phys = 0;
        ctx->cores[i].vmcs_phys = 0;
        ctx->cores[i].vmxon_region = NULL;
        ctx->cores[i].vmcs_region = NULL;
        ctx->cores[i].host_stack = 0;
        ctx->cores[i].host_cr3 = 0;
    }
    ctx->core_count = 0;
    ctx->bsp_id = 0;
    ctx->hypervisor_active = 0;
}

int cpu_detect_capabilities(void) {
    uint32_t eax, ebx, ecx, edx;

    __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    if (!(ecx & (1 << 5))) {
        return -1;
    }

    __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    if (!(ebx & (1 << 3))) {
        return -1;
    }

    return 0;
}

static void *cpu_alloc_page(void) {
    void *ptr = NULL;
    __asm__ __volatile__(
        "mov $2, %%rdi\n"
        "mov $1, %%rsi\n"
        "mov %%rax, %%rdx\n"
        "call *%%rax\n"
        : "=a"(ptr)
        : "a"(g_st->boot_services->allocate_pool)
        : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11", "memory"
    );
    
    if (ptr) {
        for (uint32_t i = 0; i < 4096; i++) {
            ((uint8_t *)ptr)[i] = 0;
        }
    }
    return ptr;
}

int cpu_init_core(cpu_context_t *ctx, uint32_t cpu_id) {
    if (!ctx || cpu_id >= CPU_MAX_CORES) return -1;
    
    cpu_state_t *cpu = &ctx->cores[cpu_id];
    
    cpu->vmxon_region = (vmxon_region_t *)cpu_alloc_page();
    if (!cpu->vmxon_region) return -1;
    cpu->vmxon_phys = (uint64_t)cpu->vmxon_region;

    cpu->vmcs_region = (vmcs_region_t *)cpu_alloc_page();
    if (!cpu->vmcs_region) return -1;
    cpu->vmcs_phys = (uint64_t)cpu->vmcs_region;

    uint64_t vmx_basic = 0;
    __asm__ __volatile__("rdmsr" : "=a"(*(uint32_t*)&vmx_basic), "=d"(*(((uint32_t*)&vmx_basic)+1)) : "c"(0x480));

    uint32_t revision_id = (uint32_t)(vmx_basic & 0xFFFFFFFF);
    cpu->vmxon_region->revision_id = revision_id;
    cpu->vmcs_region->revision_id = revision_id;

    if (__vmx_on((void *)cpu->vmxon_phys) != 0) {
        return -1;
    }
    
    if (__vmx_vmclear((void *)cpu->vmcs_phys) != 0) {
        __vmx_off();
        return -1;
    }

    if (__vmx_vmptrld((void *)cpu->vmcs_phys) != 0) {
        __vmx_off();
        return -1;
    }

    cpu->vmx_active = 1;
    ctx->core_count++;
    
    return 0;
}

int cpu_start_all(cpu_context_t *ctx) {
    if (!ctx) return -1;
    
    if (ctx->core_count == 0) {
        return -1;
    }

    for (uint32_t i = 0; i < ctx->core_count; i++) {
        if (ctx->cores[i].vmx_active) {
            continue;
        }
    }
    
    return 0;
}

int cpu_stop_all(cpu_context_t *ctx) {
    if (!ctx) return -1;
    
    for (uint32_t i = 0; i < ctx->core_count; i++) {
        if (ctx->cores[i].vmx_active) {
            __vmx_off();
            ctx->cores[i].vmx_active = 0;
        }
    }
    
    return 0;
}

cpu_state_t *cpu_get_current(cpu_context_t *ctx) {
    if (!ctx) return NULL;
    
    uint32_t cpu_id;
    __asm__ __volatile__("mov %%gs:0, %0" : "=r"(cpu_id));
    
    if (cpu_id >= CPU_MAX_CORES) return NULL;
    return &ctx->cores[cpu_id];
}

void cpu_save_guest_state(cpu_state_t *cpu) {
    if (!cpu) return;

    uint64_t cr0, cr3, cr4;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));

    cpu->guest_state.guest_cr0 = cr0;
    cpu->guest_state.guest_cr3 = cr3;
    cpu->guest_state.guest_cr4 = cr4;

    __vmx_vmwrite(0x6800, cr0);
    __vmx_vmwrite(0x6802, cr3);
    __vmx_vmwrite(0x6804, cr4);

    uint64_t rip, rsp, rflags;
    __asm__ __volatile__("lea 0(%%rip), %0" : "=r"(rip));
    __asm__ __volatile__("mov %%rsp, %0" : "=r"(rsp));
    __asm__ __volatile__("pushfq\n\tpop %0" : "=r"(rflags));

    cpu->guest_state.guest_rip = rip;
    cpu->guest_state.guest_rsp = rsp;
    cpu->guest_state.guest_rflags = rflags;

    __vmx_vmwrite(0x681E, rip);
    __vmx_vmwrite(0x681C, rsp);
    __vmx_vmwrite(0x6820, rflags);
}

void cpu_restore_host_state(cpu_state_t *cpu) {
    if (!cpu) return;

    uint64_t cr0 = cpu->guest_state.guest_cr0;
    uint64_t cr3 = cpu->guest_state.guest_cr3;
    uint64_t cr4 = cpu->guest_state.guest_cr4;

    __asm__ __volatile__("mov %0, %%cr0" :: "r"(cr0));
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(cr3));
    __asm__ __volatile__("mov %0, %%cr4" :: "r"(cr4));

    __asm__ __volatile__(
        "mov %0, %%rsp\n\t"
        "push %1\n\t"
        "popfq\n\t"
        "jmp *%2\n\t"
        :: "r"(cpu->guest_state.guest_rsp),
           "r"(cpu->guest_state.guest_rflags),
           "r"(cpu->guest_state.guest_rip)
    );
}