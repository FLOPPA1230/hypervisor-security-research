#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stddef.h>
#include "vmx.h"
#include "ept.h"
#include "msr.h"
#include "hooks.h"

#define CPU_MAX_CORES 128

typedef struct {
    uint32_t    cpu_id;
    uint32_t    vmx_active;
    uint64_t    vmxon_phys;
    uint64_t    vmcs_phys;
    vmxon_region_t *vmxon_region;
    vmcs_region_t  *vmcs_region;
    ept_context_t  ept_ctx;
    hook_context_t hook_ctx;
    msr_bitmap_t   msr_bitmap;
    uint64_t       host_stack;
    uint64_t       host_cr3;
    guest_state_t  guest_state;
} cpu_state_t;

typedef struct {
    cpu_state_t cores[CPU_MAX_CORES];
    uint32_t    core_count;
    uint32_t    bsp_id;
    uint32_t    hypervisor_active;
} cpu_context_t;

void cpu_init_context(cpu_context_t *ctx);
int  cpu_detect_capabilities(void);
int  cpu_init_core(cpu_context_t *ctx, uint32_t cpu_id);
int  cpu_start_all(cpu_context_t *ctx);
int  cpu_stop_all(cpu_context_t *ctx);
cpu_state_t *cpu_get_current(cpu_context_t *ctx);
void cpu_save_guest_state(cpu_state_t *cpu);
void cpu_restore_host_state(cpu_state_t *cpu);

#endif