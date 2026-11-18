/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file reserve.c
 * @brief First-fit allocation of kernel virtual address ranges.
 */

#include <archway/memory_layout.h>
#include <mem/mm.h>
#include <vm/reserve.h>

#define VM_MAX_RESERVATIONS 64

struct vm_reservation {
    uint64_t base;
    uint64_t size;
};

static struct vm_reservation reservations[VM_MAX_RESERVATIONS];
static size_t reservation_count;

static uint64_t
round_reservation_size(size_t length) {
    if (length > SIZE_MAX - (MM_PAGE_SIZE - 1)) {
        return 0;
    }
    return (length + MM_PAGE_SIZE - 1) & ~(MM_PAGE_SIZE - 1);
}

void *
vm_reserve(enum vm_region region, size_t length) {
    uint64_t size = round_reservation_size(length);
    if (size == 0 || reservation_count == VM_MAX_RESERVATIONS) {
        return NULL;
    }

    uint64_t region_base;
    uint64_t region_size;
    switch (region) {
    case VM_REGION_VMALLOC:
        region_base = ARCHWAY_VMALLOC_BASE;
        region_size = ARCHWAY_VMALLOC_END - ARCHWAY_VMALLOC_BASE;
        break;
    case VM_REGION_DRIVERS:
        region_base = ARCHWAY_DRIVERS_BASE;
        region_size = ARCHWAY_DRIVERS_SIZE;
        break;
    default: return NULL;
    }

    size_t index = 0;
    while (index < reservation_count && reservations[index].base < region_base) {
        ++index;
    }

    uint64_t offset = 0;
    for (; index < reservation_count; ++index) {
        uint64_t reserved_offset = reservations[index].base - region_base;
        if (reserved_offset >= region_size || reserved_offset - offset >= size) {
            break;
        }
        offset = reserved_offset + reservations[index].size;
    }
    if (region_size - offset < size) {
        return NULL;
    }

    for (size_t i = reservation_count; i > index; --i) {
        reservations[i] = reservations[i - 1];
    }
    uint64_t base = region_base + offset;
    reservations[index] = (struct vm_reservation){.base = base, .size = size};
    ++reservation_count;
    return (void *)(uintptr_t)base;
}

bool
vm_release(void *base, size_t length) {
    uint64_t address = (uintptr_t)base;
    uint64_t size = round_reservation_size(length);
    size_t index = 0;
    while (index < reservation_count && reservations[index].base < address) {
        ++index;
    }
    if (index == reservation_count || reservations[index].base != address
        || reservations[index].size != size) {
        return false;
    }

    --reservation_count;
    for (size_t i = index; i < reservation_count; ++i) {
        reservations[i] = reservations[i + 1];
    }
    return true;
}
