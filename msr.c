#include "../include/msr.h"

int msr_init_bitmap(msr_bitmap_t *bitmap) {
    if (!bitmap) return -1;
    for (uint32_t i = 0; i < MSR_BITMAP_SIZE; i++) {
        ((uint8_t *)bitmap)[i] = 0;
    }
    return 0;
}

int msr_set_bitmap_range(msr_bitmap_t *bitmap, uint32_t msr, int read, int write) {
    if (!bitmap) return -1;

    uint32_t msr_offset = msr & 0x1FFF;
    uint32_t byte_index = msr_offset / 8;
    uint32_t bit_index = msr_offset % 8;

    if (read) {
        bitmap->read_bitmap[byte_index] |= (1 << bit_index);
    }
    if (write) {
        bitmap->write_bitmap[byte_index] |= (1 << bit_index);
    }

    return 0;
}

int msr_clear_bitmap_range(msr_bitmap_t *bitmap, uint32_t msr, int read, int write) {
    if (!bitmap) return -1;

    uint32_t msr_offset = msr & 0x1FFF;
    uint32_t byte_index = msr_offset / 8;
    uint32_t bit_index = msr_offset % 8;

    if (read) {
        bitmap->read_bitmap[byte_index] &= ~(1 << bit_index);
    }
    if (write) {
        bitmap->write_bitmap[byte_index] &= ~(1 << bit_index);
    }

    return 0;
}

int msr_is_intercepted(msr_bitmap_t *bitmap, uint32_t msr, int is_write) {
    if (!bitmap) return 0;

    uint32_t msr_offset = msr & 0x1FFF;
    uint32_t byte_index = msr_offset / 8;
    uint32_t bit_index = msr_offset % 8;

    if (is_write) {
        return (bitmap->write_bitmap[byte_index] & (1 << bit_index)) ? 1 : 0;
    } else {
        return (bitmap->read_bitmap[byte_index] & (1 << bit_index)) ? 1 : 0;
    }
}

int msr_handle_exit(uint32_t msr, uint64_t *value, int is_write) {
    if (!value) return -1;

    if (!is_write) {
        if (msr == MSR_IA32_FEATURE_CONTROL) {
            *value &= ~(1ULL << 2);
            *value &= ~(1ULL << 1);
        }
        else if (msr == MSR_IA32_SYSENTER_EIP) {
            *value = 0;
        }
        else if (msr == MSR_IA32_LSTAR) {
            *value = 0;
        }
        else {
            *value = 0;
        }
    } else {
        return 0;
    }

    return 0;
}

uint64_t msr_read_native(uint32_t msr) {
    uint32_t low, high;
    __asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

void msr_write_native(uint32_t msr, uint64_t value) {
    uint32_t low = (uint32_t)(value & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(value >> 32);
    __asm__ __volatile__("wrmsr" :: "c"(msr), "a"(low), "d"(high));
}