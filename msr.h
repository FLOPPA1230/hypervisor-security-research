#ifndef MSR_H
#define MSR_H

#include <stdint.h>
#include <stddef.h>

#define MSR_IA32_FEATURE_CONTROL       0x003A
#define MSR_IA32_SYSENTER_CS           0x0174
#define MSR_IA32_SYSENTER_ESP          0x0175
#define MSR_IA32_SYSENTER_EIP          0x0176
#define MSR_IA32_LSTAR                 0xC0000082
#define MSR_IA32_FMASK                 0xC0000084
#define MSR_IA32_STAR                  0xC0000081

#define MSR_BITMAP_SIZE                4096

typedef struct __attribute__((packed)) {
    uint8_t read_bitmap[2048];
    uint8_t write_bitmap[2048];
} msr_bitmap_t;

typedef struct {
    uint64_t start;
    uint64_t end;
    uint32_t flags;
} msr_range_t;

int msr_init_bitmap(msr_bitmap_t *bitmap);
int msr_set_bitmap_range(msr_bitmap_t *bitmap, uint32_t msr, int read, int write);
int msr_clear_bitmap_range(msr_bitmap_t *bitmap, uint32_t msr, int read, int write);
int msr_is_intercepted(msr_bitmap_t *bitmap, uint32_t msr, int is_write);

int msr_handle_exit(uint32_t msr, uint64_t *value, int is_write);
uint64_t msr_read_native(uint32_t msr);
void msr_write_native(uint32_t msr, uint64_t value);

#endif