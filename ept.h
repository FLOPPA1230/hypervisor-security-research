#ifndef EPT_H
#define EPT_H

#include <stdint.h>
#include <stddef.h>

#define EPT_PML4_ENTRIES 512
#define EPT_PDPT_ENTRIES 512
#define EPT_PD_ENTRIES   512
#define EPT_PT_ENTRIES   512

#define EPT_PAGE_SIZE_4K  0x1000ULL
#define EPT_PAGE_SIZE_2M  0x200000ULL
#define EPT_PAGE_SIZE_1G  0x40000000ULL

#define EPT_MEMORY_TYPE_UC 0
#define EPT_MEMORY_TYPE_WB 6

#define EPT_ACCESS_READ    (1ULL << 0)
#define EPT_ACCESS_WRITE   (1ULL << 1)
#define EPT_ACCESS_EXECUTE (1ULL << 2)

#define EPT_ENTRY_PRESENT      (1ULL << 0)
#define EPT_ENTRY_WRITE        (1ULL << 1)
#define EPT_ENTRY_EXECUTE      (1ULL << 2)
#define EPT_ENTRY_PWT          (1ULL << 3)
#define EPT_ENTRY_PCD          (1ULL << 4)
#define EPT_ENTRY_ACCESSED     (1ULL << 8)
#define EPT_ENTRY_DIRTY        (1ULL << 9)
#define EPT_ENTRY_PS           (1ULL << 7)

#define EPT_ENTRY_PHYS_ADDR_MASK 0x000FFFFFFFFFF000ULL
#define EPT_ENTRY_IGNORE_MASK    0xFFF0000000000FFFULL

typedef struct __attribute__((packed)) {
    uint64_t entries[EPT_PML4_ENTRIES];
} ept_pml4_t;

typedef struct __attribute__((packed)) {
    uint64_t entries[EPT_PDPT_ENTRIES];
} ept_pdpt_t;

typedef struct __attribute__((packed)) {
    uint64_t entries[EPT_PD_ENTRIES];
} ept_pd_t;

typedef struct __attribute__((packed)) {
    uint64_t entries[EPT_PT_ENTRIES];
} ept_pt_t;

typedef struct __attribute__((packed)) {
    uint64_t ept_pointer;
    uint8_t  reserved[4088];
} eptp_t;

typedef struct {
    ept_pml4_t *pml4;
    ept_pdpt_t *pdpt;
    ept_pd_t   *pd;
    ept_pt_t   *pt;
    uint64_t    eptp_phys;
} ept_context_t;

void ept_init_context(ept_context_t *ctx);
int  ept_build_identity(ept_context_t *ctx, uint64_t host_cr3);
int  ept_map_page(ept_context_t *ctx, uint64_t guest_phys, uint64_t host_phys, uint64_t flags);
int  ept_unmap_page(ept_context_t *ctx, uint64_t guest_phys);
int  ept_split_page(ept_context_t *ctx, uint64_t guest_phys);
uint64_t ept_get_entry(ept_context_t *ctx, uint64_t guest_phys);
uint64_t ept_get_eptp(ept_context_t *ctx);

#endif