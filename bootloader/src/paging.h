/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file paging.h
 * @brief LoongArch page table inf.
 */

#ifndef ARCHWAY_PAGING_H
#define ARCHWAY_PAGING_H
#include "paging_layout.h"
#include <efi.h>

struct page_map {
    efi_physical_address root;
};

efi_status page_map_init(struct efi_boot_services *bs, struct page_map *map);
efi_status page_map_range(
    struct efi_boot_services *bs,
    struct page_map *map,
    uint64_t virtual,
    uint64_t physical,
    size_t size,
    uint32_t flags
);
void page_map_destroy(struct efi_boot_services *bs, struct page_map *map);
#endif
