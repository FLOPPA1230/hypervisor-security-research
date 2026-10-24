#include "../include/ept.h"

extern void *efi_alloc_pages(uint64_t pages);
extern int __invept(uint64_t type, void *descriptor);

typedef struct __attribute__((packed)) {
    uint64_t eptp;
    uint64_t reserved;
} invept_desc_t;

static void *ept_alloc_table(void) {
    return efi_alloc_pages(1);
}

void ept_init_context(ept_context_t *ctx) {
    if (!ctx) return;
    ctx->pml4 = NULL;
    ctx->pdpt = NULL;
    ctx->pd = NULL;
    ctx->pt = NULL;
    ctx->eptp_phys = 0;
}

int ept_build_identity(ept_context_t *ctx, uint64_t host_cr3) {
    (void)host_cr3;
    if (!ctx) return -1;

    ctx->pml4 = (ept_pml4_t *)ept_alloc_table();
    if (!ctx->pml4) return -1;

    ctx->pdpt = (ept_pdpt_t *)ept_alloc_table();
    if (!ctx->pdpt) return -1;

    ctx->pd = (ept_pd_t *)ept_alloc_table();
    if (!ctx->pd) return -1;

    ctx->pt = (ept_pt_t *)ept_alloc_table();
    if (!ctx->pt) return -1;

    ctx->pml4->entries[0] = ((uint64_t)ctx->pdpt & EPT_ENTRY_PHYS_ADDR_MASK) | EPT_ENTRY_PRESENT | EPT_ENTRY_WRITE | EPT_ENTRY_EXECUTE;
    ctx->pdpt->entries[0] = ((uint64_t)ctx->pd & EPT_ENTRY_PHYS_ADDR_MASK) | EPT_ENTRY_PRESENT | EPT_ENTRY_WRITE | EPT_ENTRY_EXECUTE;
    ctx->pd->entries[0] = ((uint64_t)ctx->pt & EPT_ENTRY_PHYS_ADDR_MASK) | EPT_ENTRY_PRESENT | EPT_ENTRY_WRITE | EPT_ENTRY_EXECUTE;

    for (uint32_t i = 0; i < EPT_PT_ENTRIES; i++) {
        uint64_t phys = (uint64_t)i * EPT_PAGE_SIZE_4K;
        ctx->pt->entries[i] = phys | EPT_ENTRY_PRESENT | EPT_ENTRY_WRITE | EPT_ENTRY_EXECUTE;
    }

    ctx->eptp_phys = (uint64_t)ctx->pml4;

    invept_desc_t desc;
    desc.eptp = ctx->eptp_phys | VMX_EPT_DEFAULT_MEMORY_TYPE | (3ULL << 3);
    desc.reserved = 0;
    
    __invept(1, &desc);

    return 0;
}

int ept_map_page(ept_context_t *ctx, uint64_t guest_phys, uint64_t host_phys, uint64_t flags) {
    if (!ctx || !ctx->pml4) return -1;

    uint64_t pml4_idx = (guest_phys >> 39) & 0x1FF;
    uint64_t pdpt_idx = (guest_phys >> 30) & 0x1FF;
    uint64_t pd_idx   = (guest_phys >> 21) & 0x1FF;
    uint64_t pt_idx   = (guest_phys >> 12) & 0x1FF;

    ept_pdpt_t *pdpt = NULL;
    if (!(ctx->pml4->entries[pml4_idx] & EPT_ENTRY_PRESENT)) {
        pdpt = (ept_pdpt_t *)ept_alloc_table();
        if (!pdpt) return -1;
        ctx->pml4->entries[pml4_idx] = ((uint64_t)pdpt & EPT_ENTRY_PHYS_ADDR_MASK) | EPT_ENTRY_PRESENT | EPT_ENTRY_WRITE | EPT_ENTRY_EXECUTE;
    } else {
        pdpt = (ept_pdpt_t *)(ctx->pml4->entries[pml4_idx] & EPT_ENTRY_PHYS_ADDR_MASK);
    }

    ept_pd_t *pd = NULL;
    if (!(pdpt->entries[pdpt_idx] & EPT_ENTRY_PRESENT)) {
        pd = (ept_pd_t *)ept_alloc_table();
        if (!pd) return -1;
        pdpt->entries[pdpt_idx] = ((uint64_t)pd & EPT_ENTRY_PHYS_ADDR_MASK) | EPT_ENTRY_PRESENT | EPT_ENTRY_WRITE | EPT_ENTRY_EXECUTE;
    } else {
        pd = (ept_pd_t *)(pdpt->entries[pdpt_idx] & EPT_ENTRY_PHYS_ADDR_MASK);
    }

    ept_pt_t *pt = NULL;
    if (!(pd->entries[pd_idx] & EPT_ENTRY_PRESENT)) {
        pt = (ept_pt_t *)ept_alloc_table();
        if (!pt) return -1;
        pd->entries[pd_idx] = ((uint64_t)pt & EPT_ENTRY_PHYS_ADDR_MASK) | EPT_ENTRY_PRESENT | EPT_ENTRY_WRITE | EPT_ENTRY_EXECUTE;
    } else {
        pt = (ept_pt_t *)(pd->entries[pd_idx] & EPT_ENTRY_PHYS_ADDR_MASK);
    }

    uint64_t entry = (host_phys & EPT_ENTRY_PHYS_ADDR_MASK) | EPT_ENTRY_PRESENT;
    if (flags & EPT_ACCESS_READ)    entry |= EPT_ENTRY_PRESENT;
    if (flags & EPT_ACCESS_WRITE)   entry |= EPT_ENTRY_WRITE;
    if (flags & EPT_ACCESS_EXECUTE) entry |= EPT_ENTRY_EXECUTE;

    pt->entries[pt_idx] = entry;

    invept_desc_t desc;
    desc.eptp = ctx->eptp_phys | VMX_EPT_DEFAULT_MEMORY_TYPE | (3ULL << 3);
    desc.reserved = 0;
    __invept(1, &desc);

    return 0;
}

int ept_unmap_page(ept_context_t *ctx, uint64_t guest_phys) {
    if (!ctx || !ctx->pml4) return -1;

    uint64_t pml4_idx = (guest_phys >> 39) & 0x1FF;
    uint64_t pdpt_idx = (guest_phys >> 30) & 0x1FF;
    uint64_t pd_idx   = (guest_phys >> 21) & 0x1FF;
    uint64_t pt_idx   = (guest_phys >> 12) & 0x1FF;

    if (!(ctx->pml4->entries[pml4_idx] & EPT_ENTRY_PRESENT)) return -1;
    ept_pdpt_t *pdpt = (ept_pdpt_t *)(ctx->pml4->entries[pml4_idx] & EPT_ENTRY_PHYS_ADDR_MASK);

    if (!(pdpt->entries[pdpt_idx] & EPT_ENTRY_PRESENT)) return -1;
    ept_pd_t *pd = (ept_pd_t *)(pdpt->entries[pdpt_idx] & EPT_ENTRY_PHYS_ADDR_MASK);

    if (!(pd->entries[pd_idx] & EPT_ENTRY_PRESENT)) return -1;
    ept_pt_t *pt = (ept_pt_t *)(pd->entries[pd_idx] & EPT_ENTRY_PHYS_ADDR_MASK);

    pt->entries[pt_idx] = 0;

    invept_desc_t desc;
    desc.eptp = ctx->eptp_phys | VMX_EPT_DEFAULT_MEMORY_TYPE | (3ULL << 3);
    desc.reserved = 0;
    __invept(1, &desc);

    return 0;
}

int ept_split_page(ept_context_t *ctx, uint64_t guest_phys) {
    (void)ctx;
    (void)guest_phys;
    return 0;
}

uint64_t ept_get_entry(ept_context_t *ctx, uint64_t guest_phys) {
    if (!ctx || !ctx->pml4) return 0;

    uint64_t pml4_idx = (guest_phys >> 39) & 0x1FF;
    uint64_t pdpt_idx = (guest_phys >> 30) & 0x1FF;
    uint64_t pd_idx   = (guest_phys >> 21) & 0x1FF;
    uint64_t pt_idx   = (guest_phys >> 12) & 0x1FF;

    if (!(ctx->pml4->entries[pml4_idx] & EPT_ENTRY_PRESENT)) return 0;
    ept_pdpt_t *pdpt = (ept_pdpt_t *)(ctx->pml4->entries[pml4_idx] & EPT_ENTRY_PHYS_ADDR_MASK);

    if (!(pdpt->entries[pdpt_idx] & EPT_ENTRY_PRESENT)) return 0;
    ept_pd_t *pd = (ept_pd_t *)(pdpt->entries[pdpt_idx] & EPT_ENTRY_PHYS_ADDR_MASK);

    if (!(pd->entries[pd_idx] & EPT_ENTRY_PRESENT)) return 0;
    ept_pt_t *pt = (ept_pt_t *)(pd->entries[pd_idx] & EPT_ENTRY_PHYS_ADDR_MASK);

    return pt->entries[pt_idx];
}

uint64_t ept_get_eptp(ept_context_t *ctx) {
    if (!ctx) return 0;
    return ctx->eptp_phys | VMX_EPT_DEFAULT_MEMORY_TYPE | (3ULL << 3);
}