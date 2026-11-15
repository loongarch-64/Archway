/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file paging.c
 * @brief Page table construction.
 */

#include "paging.h"
#include "elf.h"
#include "string.h"

#define PTE_VALID UINT64_C(1)
#define PTE_DIRTY UINT64_C(2)
#define PTE_CACHED UINT64_C(0x10)
#define PTE_GLOBAL UINT64_C(0x40)
#define PTE_NO_READ (UINT64_C(1) << 61)
#define PTE_NO_EXEC (UINT64_C(1) << 62)
#define PTE_PHYSICAL_MASK UINT64_C(0x0000fffffffff000)

static efi_status
allocate_table(struct efi_boot_services *bs, efi_physical_address *physical, bool leaf) {
    efi_status status = bs->allocate_pages(EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_DATA, 1, physical);
    if (EFI_ERROR(status)) {
        return status;
    }
    uint64_t *table = (void *)(uintptr_t)*physical;
    for (size_t i = 0; i < 512; ++i) {
        table[i] = leaf ? PTE_GLOBAL : 0;
    }
    return EFI_SUCCESS;
}

efi_status
page_map_init(struct efi_boot_services *bs, struct page_map *map) {
    map->root = 0;
    return allocate_table(bs, &map->root, false);
}

static void
destroy_table(struct efi_boot_services *bs, uint64_t physical, unsigned level) {
    uint64_t *table = (void *)(uintptr_t)physical;
    if (level != 0) {
        for (size_t i = 0; i < 512; ++i) {
            if (table[i] != 0) {
                destroy_table(bs, table[i], level - 1);
            }
        }
    }
    bs->free_pages(physical, 1);
}

void
page_map_destroy(struct efi_boot_services *bs, struct page_map *map) {
    if (map->root != 0) {
        destroy_table(bs, map->root, 3);
    }
    map->root = 0;
}

efi_status
page_map_range(
    struct efi_boot_services *bs,
    struct page_map *map,
    uint64_t virtual,
    uint64_t physical,
    size_t size,
    uint32_t flags
) {
    if (size == 0 || map->root == 0 || ((virtual | physical | size) & 0xfff) != 0
        || size - 1 > UINT64_MAX - virtual || physical > PTE_PHYSICAL_MASK
        || size - 1 > (PTE_PHYSICAL_MASK | 0xfff) - physical) {
        return EFI_INVALID_PARAMETER;
    }
    uint64_t attributes = PTE_VALID | PTE_CACHED | PTE_GLOBAL;
    if ((flags & ELF_PF_W) != 0) {
        attributes |= PTE_DIRTY;
    }
    if ((flags & ELF_PF_R) == 0) {
        attributes |= PTE_NO_READ;
    }
    if ((flags & ELF_PF_X) == 0) {
        attributes |= PTE_NO_EXEC;
    }
    for (size_t offset = 0; offset < size; offset += EFI_PAGE_SIZE) {
        uint64_t address = virtual + offset;
        uint64_t *table = (void *)(uintptr_t)map->root;
        for (unsigned level = 3; level != 0; --level) {
            size_t index = (address >> (12 + level * 9)) & 511;
            if (table[index] == 0) {
                efi_physical_address next;
                efi_status status = allocate_table(bs, &next, level == 1);
                if (EFI_ERROR(status)) {
                    return status;
                }
                table[index] = next;
            }
            table = (void *)(uintptr_t)table[index];
        }
        uint64_t *pte = &table[(address >> 12) & 511];
        if ((*pte & PTE_VALID) != 0) {
            if ((*pte & PTE_PHYSICAL_MASK) != physical + offset) {
                return EFI_LOAD_ERROR;
            }
            uint64_t dirty = (*pte | attributes) & PTE_DIRTY;
            uint64_t denied = (*pte & attributes) & (PTE_NO_READ | PTE_NO_EXEC);
            *pte = (physical + offset) | PTE_VALID | PTE_CACHED | PTE_GLOBAL | dirty | denied;
        } else {
            *pte = (physical + offset) | attributes;
        }
    }
    return EFI_SUCCESS;
}
