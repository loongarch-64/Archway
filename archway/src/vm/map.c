/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file map.c
 * @brief Kernel virtual-memory mapping policy.
 */

#include <arch/paging.h>
#include <vm/map.h>

#define VM_PAGE_SIZE UINT64_C(4096)
#define VM_PAGE_MASK (VM_PAGE_SIZE - 1)
#define VM_MAP_FLAG_MASK ((uint32_t)(VM_MAP_READ | VM_MAP_WRITE | VM_MAP_EXECUTE | VM_MAP_UNCACHED))

static enum vm_map_result
translate_paging_result(enum paging_result result) {
    switch (result) {
    case PAGING_OK: return VM_MAP_OK;
    case PAGING_INVALID_ARGUMENT: return VM_MAP_INVALID_ARGUMENT;
    case PAGING_NO_MEMORY: return VM_MAP_NO_MEMORY;
    case PAGING_CONFLICT: return VM_MAP_CONFLICT;
    case PAGING_UNSUPPORTED: return VM_MAP_UNSUPPORTED;
    }
    return VM_MAP_INVALID_ARGUMENT;
}

static enum paging_flags
translate_map_flags(enum vm_map_flags flags) {
    uint32_t paging_flags = 0;
    if ((flags & VM_MAP_READ) != 0) {
        paging_flags |= PAGING_READ;
    }
    if ((flags & VM_MAP_WRITE) != 0) {
        paging_flags |= PAGING_WRITE;
    }
    if ((flags & VM_MAP_EXECUTE) != 0) {
        paging_flags |= PAGING_EXECUTE;
    }
    if ((flags & VM_MAP_UNCACHED) != 0) {
        paging_flags |= PAGING_UNCACHED;
    }
    return (enum paging_flags)paging_flags;
}

enum vm_map_result
vm_map_range(
    uint64_t virtual_address, uint64_t physical_address, size_t size, enum vm_map_flags flags
) {
    if (size == 0 || ((virtual_address | physical_address | size) & VM_PAGE_MASK) != 0
        || (uint32_t)flags & ~VM_MAP_FLAG_MASK) {
        return VM_MAP_INVALID_ARGUMENT;
    }
    if ((uint64_t)(size - 1) > UINT64_MAX - virtual_address
        || (uint64_t)(size - 1) > UINT64_MAX - physical_address) {
        return VM_MAP_INVALID_ARGUMENT;
    }

    enum paging_flags paging_flags = translate_map_flags(flags);
    for (uint64_t offset = 0; offset < (uint64_t)size; offset += VM_PAGE_SIZE) {
        enum paging_result result =
            paging_map_page(virtual_address + offset, physical_address + offset, paging_flags);
        if (result != PAGING_OK) {
            return translate_paging_result(result);
        }
    }
    return VM_MAP_OK;
}

enum vm_map_result
vm_unmap_range(uint64_t virtual_address, size_t size) {
    if (size == 0 || ((virtual_address | size) & VM_PAGE_MASK) != 0
        || (uint64_t)(size - 1) > UINT64_MAX - virtual_address) {
        return VM_MAP_INVALID_ARGUMENT;
    }

    for (uint64_t offset = 0; offset < (uint64_t)size; offset += VM_PAGE_SIZE) {
        enum paging_result result = paging_unmap_page(virtual_address + offset);
        if (result != PAGING_OK) {
            return translate_paging_result(result);
        }
    }
    return VM_MAP_OK;
}
