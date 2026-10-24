#include "../include/vmx.h"
#include "../include/cpu.h"
#include "../include/msr.h"
#include "../include/ept.h"

extern int __vmx_vmread(uint64_t field, uint64_t *value);
extern int __vmx_vmwrite(uint64_t field, uint64_t value);
extern int __vmlaunch(void);
extern int __vmresume(void);
extern int __vmx_on(void *vmxon_phys);
extern int __vmx_off(void);

static uint64_t g_host_stack[4096] __attribute__((aligned(16)));

uint8_t vmx_vmread8(uint64_t field) {
    uint64_t value = 0;
    __vmx_vmread(field, &value);
    return (uint8_t)value;
}

uint16_t vmx_vmread16(uint64_t field) {
    uint64_t value = 0;
    __vmx_vmread(field, &value);
    return (uint16_t)value;
}

uint32_t vmx_vmread32(uint64_t field) {
    uint64_t value = 0;
    __vmx_vmread(field, &value);
    return (uint32_t)value;
}

uint64_t vmx_vmread64(uint64_t field) {
    uint64_t value = 0;
    __vmx_vmread(field, &value);
    return value;
}

void vmx_vmwrite8(uint64_t field, uint8_t value) {
    __vmx_vmwrite(field, (uint64_t)value);
}

void vmx_vmwrite16(uint64_t field, uint16_t value) {
    __vmx_vmwrite(field, (uint64_t)value);
}

void vmx_vmwrite32(uint64_t field, uint32_t value) {
    __vmx_vmwrite(field, (uint64_t)value);
}

void vmx_vmwrite64(uint64_t field, uint64_t value) {
    __vmx_vmwrite(field, value);
}

int vmx_enable(void) {
    uint64_t cr4;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 13);
    __asm__ __volatile__("mov %0, %%cr4" :: "r"(cr4));

    uint64_t feature_control = msr_read_native(0x3A);
    if (!(feature_control & (1ULL << 2))) {
        feature_control |= (1ULL << 2);
        msr_write_native(0x3A, feature_control);
    }

    return VMX_ERR_SUCCESS;
}

void vmx_disable(void) {
    uint64_t cr4;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
    cr4 &= ~(1ULL << 13);
    __asm__ __volatile__("mov %0, %%cr4" :: "r"(cr4));
}

static void vmx_setup_vmcs(cpu_state_t *cpu) {
    uint64_t cr0, cr3, cr4;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));

    vmx_vmwrite64(VMCS_FIELD_HOST_CR0, cr0);
    vmx_vmwrite64(VMCS_FIELD_HOST_CR3, cr3);
    vmx_vmwrite64(VMCS_FIELD_HOST_CR4, cr4);

    uint16_t cs, ss, ds, es, fs, gs, tr;
    __asm__ __volatile__("mov %%cs, %0" : "=r"(cs));
    __asm__ __volatile__("mov %%ss, %0" : "=r"(ss));
    __asm__ __volatile__("mov %%ds, %0" : "=r"(ds));
    __asm__ __volatile__("mov %%es, %0" : "=r"(es));
    __asm__ __volatile__("mov %%fs, %0" : "=r"(fs));
    __asm__ __volatile__("mov %%gs, %0" : "=r"(gs));
    __asm__ __volatile__("str %0" : "=r"(tr));

    vmx_vmwrite16(VMCS_FIELD_HOST_CS_SEL, cs);
    vmx_vmwrite16(VMCS_FIELD_HOST_SS_SEL, ss);
    vmx_vmwrite16(VMCS_FIELD_HOST_DS_SEL, ds);
    vmx_vmwrite16(VMCS_FIELD_HOST_ES_SEL, es);
    vmx_vmwrite16(VMCS_FIELD_HOST_FS_SEL, fs);
    vmx_vmwrite16(VMCS_FIELD_HOST_GS_SEL, gs);
    vmx_vmwrite16(VMCS_FIELD_HOST_TR_SEL, tr);

    uint64_t host_rip;
    __asm__ __volatile__("lea 0(%%rip), %0" : "=r"(host_rip));
    vmx_vmwrite64(VMCS_FIELD_HOST_RIP, host_rip);
    vmx_vmwrite64(VMCS_FIELD_HOST_RSP, (uint64_t)&g_host_stack[4095]);

    vmx_vmwrite64(VMCS_FIELD_GUEST_CR0, cr0);
    vmx_vmwrite64(VMCS_FIELD_GUEST_CR3, cr3);
    vmx_vmwrite64(VMCS_FIELD_GUEST_CR4, cr4);

    uint64_t guest_rip;
    __asm__ __volatile__("lea 0(%%rip), %0" : "=r"(guest_rip));
    vmx_vmwrite64(VMCS_FIELD_GUEST_RIP, guest_rip);
    vmx_vmwrite64(VMCS_FIELD_GUEST_RSP, (uint64_t)&g_host_stack[4095]);
    vmx_vmwrite64(VMCS_FIELD_GUEST_RFLAGS, 0x2);

    uint64_t eptp = ept_get_eptp(&cpu->ept_ctx);
    vmx_vmwrite64(VMCS_FIELD_CTRL_EPTP, eptp);

    uint64_t procbased = 0;
    procbased |= (1ULL << 31);
    procbased |= (1ULL << 28);
    procbased |= (1ULL << 15);
    procbased |= (1ULL << 7);
    procbased |= (1ULL << 21);
    vmx_vmwrite32(VMCS_FIELD_CTRL_CPU_BASED, procbased);

    vmx_vmwrite32(VMCS_FIELD_CTRL_PIN_BASED, 0);

    uint64_t exit_ctls = 0;
    exit_ctls |= (1ULL << 9);
    vmx_vmwrite32(VMCS_FIELD_CTRL_EXIT, exit_ctls);

    vmx_vmwrite32(VMCS_FIELD_CTRL_ENTRY, 0);
    vmx_vmwrite64(VMCS_FIELD_CTRL_MSR_BITMAPS, (uint64_t)&cpu->msr_bitmap);
}

static void vmx_handle_exit(cpu_state_t *cpu, uint64_t exit_reason, uint64_t exit_qualification) {
    (void)exit_qualification;
    
    switch (exit_reason) {
        case VMX_EXIT_REASON_CPUID: {
            uint32_t eax = (uint32_t)vmx_vmread64(0x6800);
            uint32_t ecx = (uint32_t)vmx_vmread64(0x6810);
            uint32_t ebx, edx;
            __asm__ __volatile__("cpuid" : "=b"(ebx), "=d"(edx) : "a"(eax), "c"(ecx));
            vmx_vmwrite64(0x6800, eax);
            vmx_vmwrite64(0x6808, ebx);
            vmx_vmwrite64(0x6810, ecx);
            vmx_vmwrite64(0x6818, edx);
            break;
        }
        case VMX_EXIT_REASON_RDMSR: {
            uint32_t msr = (uint32_t)vmx_vmread64(0x6800);
            uint64_t value = 0;
            msr_handle_exit(msr, &value, 0);
            vmx_vmwrite64(0x6800, (uint32_t)value);
            vmx_vmwrite64(0x6808, (uint32_t)(value >> 32));
            break;
        }
        case VMX_EXIT_REASON_WRMSR: {
            uint32_t msr = (uint32_t)vmx_vmread64(0x6800);
            uint64_t value = vmx_vmread64(0x6808) | ((uint64_t)vmx_vmread64(0x6810) << 32);
            msr_handle_exit(msr, &value, 1);
            break;
        }
        case VMX_EXIT_REASON_EPT_VIOLATION: {
            hook_exit_info_t info;
            info.guest_phys = vmx_vmread64(0x2400);
            info.access_flags = vmx_vmread64(0x6400);
            info.exit_qualification = exit_qualification;
            uint64_t new_cr3 = 0;
            hooks_handle_ept_violation(&cpu->hook_ctx, &info, &new_cr3);
            if (new_cr3) {
                vmx_vmwrite64(VMCS_FIELD_GUEST_CR3, new_cr3);
            }
            break;
        }
        default:
            break;
    }
}

int vmx_launch(cpu_state_t *cpu) {
    if (!cpu) return VMX_ERR_FAILURE;
    
    vmx_enable();
    vmx_setup_vmcs(cpu);

    int status = __vmlaunch();
    
    if (status != 0) {
        while (1) {
            uint64_t exit_reason = vmx_vmread64(0x4402);
            uint64_t exit_qualification = vmx_vmread64(0x6400);
            vmx_handle_exit(cpu, exit_reason, exit_qualification);
            status = __vmresume();
            if (status != 0) {
                break;
            }
        }
    }

    vmx_disable();
    return VMX_ERR_SUCCESS;
}

int vmx_vmclear(void *vmcs_phys) {
    extern int __vmx_vmclear(void *vmcs_phys);
    return __vmx_vmclear(vmcs_phys);
}

int vmx_vmptrld(void *vmcs_phys) {
    extern int __vmx_vmptrld(void *vmcs_phys);
    return __vmx_vmptrld(vmcs_phys);
}