#include <stdint.h>
#include <stddef.h>
#include "../include/cpu.h"
#include "../include/vmx.h"
#include "../include/ept.h"
#include "../include/msr.h"
#include "../include/hooks.h"

typedef uint64_t efi_status_t;
typedef void* efi_handle_t;

#define EFI_SUCCESS 0
#define EFI_ERROR   1

typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
} efi_table_header_t;

typedef struct {
    efi_table_header_t hdr;
    void (*raise_tpl)(uint64_t);
    void (*restore_tpl)(uint64_t);
    efi_status_t (*allocate_pages)(uint32_t, uint32_t, uint64_t, uint64_t*);
    efi_status_t (*free_pages)(uint64_t, uint64_t);
    efi_status_t (*get_memory_map)(uint64_t*, void*, uint64_t*, uint64_t*, uint32_t*);
    efi_status_t (*allocate_pool)(uint32_t, uint64_t, void**);
    efi_status_t (*free_pool)(void*);
    void (*output_string)(void*, const uint16_t*);
} efi_boot_services_t;

typedef struct {
    efi_table_header_t hdr;
    uint16_t *firmware_vendor;
    uint32_t firmware_revision;
    efi_handle_t console_in_handle;
    void *con_in;
    efi_handle_t console_out_handle;
    void *con_out;
    efi_handle_t standard_error_handle;
    void *std_err;
    void *runtime_services;
    efi_boot_services_t *boot_services;
    uint64_t num_table_entries;
    void *configuration_table;
} efi_system_table_t;

#define EFI_ALLOCATE_ANY_PAGES 0
#define EFI_ALLOCATE_MAX_ADDRESS 1
#define EFI_ALLOCATE_ADDRESS 2
#define EFI_RESERVED_MEMORY_TYPE 0
#define EFI_LOADER_DATA 2
#define EFI_CONVENTIONAL_MEMORY 7

efi_system_table_t *g_st = NULL;
static cpu_context_t g_cpu_ctx;
ept_context_t *g_ept_ctx = NULL;

void efi_print(const uint16_t *str) {
    if (g_st && g_st->con_out) {
        g_st->output_string(g_st->con_out, str);
    }
}

void *efi_alloc_pages(uint64_t pages) {
    uint64_t addr = 0;
    efi_status_t status = g_st->boot_services->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_DATA, pages, &addr);
    if (status != EFI_SUCCESS) return NULL;
    
    uint8_t *ptr = (uint8_t *)addr;
    for (uint64_t i = 0; i < pages * 4096; i++) {
        ptr[i] = 0;
    }
    return (void *)addr;
}

efi_status_t efi_main(efi_handle_t image_handle, efi_system_table_t *system_table) {
    (void)image_handle;
    g_st = system_table;

    efi_print(L"Stealth Hypervisor Loading...\r\n");

    if (cpu_detect_capabilities() != 0) {
        efi_print(L"Error: CPU does not support VMX or EPT.\r\n");
        return EFI_ERROR;
    }
    efi_print(L"CPU supports VMX and EPT.\r\n");

    cpu_init_context(&g_cpu_ctx);
    g_cpu_ctx.hypervisor_active = 1;

    if (cpu_init_core(&g_cpu_ctx, 0) != 0) {
        efi_print(L"Error: Failed to initialize BSP core.\r\n");
        return EFI_ERROR;
    }
    efi_print(L"BSP Core Initialized.\r\n");

    cpu_state_t *bsp = &g_cpu_ctx.cores[0];
    
    if (ept_build_identity(&bsp->ept_ctx, 0) != 0) {
        efi_print(L"Error: Failed to build EPT identity map.\r\n");
        return EFI_ERROR;
    }
    efi_print(L"EPT Built.\r\n");

    g_ept_ctx = &bsp->ept_ctx;

    if (vmx_launch(bsp) != 0) {
        efi_print(L"Error: Failed to launch VMX.\r\n");
        return EFI_ERROR;
    }

    efi_print(L"Stealth Hypervisor Active in Ring -1.\r\n");
    
    for (;;) {
        __asm__ __volatile__("hlt");
    }

    return EFI_SUCCESS;
}