#ifndef HOOKS_H
#define HOOKS_H

#include <stdint.h>
#include <stddef.h>

#define HOOK_MAX_PAGES 64

#define HOOK_TYPE_EPT_EXECUTE 0
#define HOOK_TYPE_EPT_READ    1
#define HOOK_TYPE_EPT_WRITE   2

#define HOOK_FLAG_ACTIVE      1
#define HOOK_FLAG_SHADOW      2

typedef struct {
    uint64_t guest_phys;
    uint64_t original_phys;
    uint64_t shadow_phys;
    uint32_t flags;
    uint32_t type;
} hook_page_t;

typedef struct {
    hook_page_t pages[HOOK_MAX_PAGES];
    uint32_t    count;
} hook_context_t;

typedef struct __attribute__((packed)) {
    uint64_t guest_phys;
    uint64_t access_flags;
    uint64_t exit_qualification;
} hook_exit_info_t;

void hooks_init(hook_context_t *ctx);
int  hooks_install_read(hook_context_t *ctx, uint64_t guest_phys, uint64_t original_phys);
int  hooks_install_execute(hook_context_t *ctx, uint64_t guest_phys, uint64_t original_phys);
int  hooks_remove(hook_context_t *ctx, uint64_t guest_phys);
hook_page_t *hooks_find(hook_context_t *ctx, uint64_t guest_phys);
int  hooks_handle_ept_violation(hook_context_t *ctx, hook_exit_info_t *exit_info, uint64_t *host_cr3);

#endif