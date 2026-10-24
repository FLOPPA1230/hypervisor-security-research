#include "../include/hooks.h"
#include "../include/ept.h"

void hooks_init(hook_context_t *ctx) {
    if (!ctx) return;
    for (uint32_t i = 0; i < HOOK_MAX_PAGES; i++) {
        ctx->pages[i].guest_phys = 0;
        ctx->pages[i].original_phys = 0;
        ctx->pages[i].shadow_phys = 0;
        ctx->pages[i].flags = 0;
        ctx->pages[i].type = 0;
    }
    ctx->count = 0;
}

hook_page_t *hooks_find(hook_context_t *ctx, uint64_t guest_phys) {
    if (!ctx) return NULL;
    uint64_t aligned_phys = guest_phys & ~0xFFFULL;
    for (uint32_t i = 0; i < ctx->count; i++) {
        if (ctx->pages[i].guest_phys == aligned_phys) {
            return &ctx->pages[i];
        }
    }
    return NULL;
}

static void *hooks_alloc_shadow_page(void) {
    extern void *efi_alloc_pages(uint64_t pages);
    void *ptr = efi_alloc_pages(1);
    if (ptr) {
        for (uint32_t i = 0; i < 4096; i++) {
            ((uint8_t *)ptr)[i] = 0;
        }
    }
    return ptr;
}

int hooks_install_execute(hook_context_t *ctx, uint64_t guest_phys, uint64_t original_phys) {
    if (!ctx) return -1;
    
    uint64_t aligned_phys = guest_phys & ~0xFFFULL;
    uint64_t aligned_orig = original_phys & ~0xFFFULL;

    if (hooks_find(ctx, aligned_phys)) return -1;
    if (ctx->count >= HOOK_MAX_PAGES) return -1;

    void *shadow = hooks_alloc_shadow_page();
    if (!shadow) return -1;

    uint8_t *src = (uint8_t *)aligned_orig;
    uint8_t *dst = (uint8_t *)shadow;
    for (uint32_t i = 0; i < 4096; i++) {
        dst[i] = src[i];
    }

    hook_page_t *page = &ctx->pages[ctx->count];
    page->guest_phys = aligned_phys;
    page->original_phys = aligned_orig;
    page->shadow_phys = (uint64_t)shadow;
    page->flags = HOOK_FLAG_ACTIVE | HOOK_FLAG_SHADOW;
    page->type = HOOK_TYPE_EPT_EXECUTE;

    ctx->count++;
    return 0;
}

int hooks_install_read(hook_context_t *ctx, uint64_t guest_phys, uint64_t original_phys) {
    if (!ctx) return -1;
    
    uint64_t aligned_phys = guest_phys & ~0xFFFULL;
    uint64_t aligned_orig = original_phys & ~0xFFFULL;

    if (hooks_find(ctx, aligned_phys)) return -1;
    if (ctx->count >= HOOK_MAX_PAGES) return -1;

    void *shadow = hooks_alloc_shadow_page();
    if (!shadow) return -1;

    uint8_t *src = (uint8_t *)aligned_orig;
    uint8_t *dst = (uint8_t *)shadow;
    for (uint32_t i = 0; i < 4096; i++) {
        dst[i] = src[i];
    }

    hook_page_t *page = &ctx->pages[ctx->count];
    page->guest_phys = aligned_phys;
    page->original_phys = aligned_orig;
    page->shadow_phys = (uint64_t)shadow;
    page->flags = HOOK_FLAG_ACTIVE | HOOK_FLAG_SHADOW;
    page->type = HOOK_TYPE_EPT_READ;

    ctx->count++;
    return 0;
}

int hooks_remove(hook_context_t *ctx, uint64_t guest_phys) {
    if (!ctx) return -1;
    
    uint64_t aligned_phys = guest_phys & ~0xFFFULL;
    for (uint32_t i = 0; i < ctx->count; i++) {
        if (ctx->pages[i].guest_phys == aligned_phys) {
            for (uint32_t j = i; j < ctx->count - 1; j++) {
                ctx->pages[j] = ctx->pages[j + 1];
            }
            ctx->count--;
            return 0;
        }
    }
    return -1;
}

int hooks_handle_ept_violation(hook_context_t *ctx, hook_exit_info_t *exit_info, uint64_t *host_cr3) {
    if (!ctx || !exit_info) return -1;

    uint64_t fault_phys = exit_info->guest_phys & ~0xFFFULL;
    hook_page_t *page = hooks_find(ctx, fault_phys);
    
    if (!page) {
        return -1;
    }

    int is_execute = (exit_info->access_flags & 0x4) ? 1 : 0;
    int is_write   = (exit_info->access_flags & 0x2) ? 1 : 0;
    int is_read    = (exit_info->access_flags & 0x1) ? 1 : 0;

    if (page->type == HOOK_TYPE_EPT_EXECUTE) {
        if (is_execute) {
            extern int ept_map_page(ept_context_t *ctx, uint64_t guest_phys, uint64_t host_phys, uint64_t flags);
            extern ept_context_t *g_ept_ctx;
            
            ept_map_page(g_ept_ctx, fault_phys, page->shadow_phys, 
                         EPT_ACCESS_READ | EPT_ACCESS_WRITE | EPT_ACCESS_EXECUTE);
        } else if (is_write) {
            extern int ept_map_page(ept_context_t *ctx, uint64_t guest_phys, uint64_t host_phys, uint64_t flags);
            extern ept_context_t *g_ept_ctx;
            
            ept_map_page(g_ept_ctx, fault_phys, page->original_phys, 
                         EPT_ACCESS_READ | EPT_ACCESS_WRITE);
        }
    } 
    else if (page->type == HOOK_TYPE_EPT_READ) {
        if (is_read) {
            extern int ept_map_page(ept_context_t *ctx, uint64_t guest_phys, uint64_t host_phys, uint64_t flags);
            extern ept_context_t *g_ept_ctx;
            
            ept_map_page(g_ept_ctx, fault_phys, page->shadow_phys, 
                         EPT_ACCESS_READ | EPT_ACCESS_WRITE);
        }
    }

    if (host_cr3) {
        *host_cr3 = 0;
    }

    return 0;
}