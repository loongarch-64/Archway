/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file kernel.h
 * @brief Kernel loading.
 */

#ifndef ARCHWAY_KERNEL_H
#define ARCHWAY_KERNEL_H
#include "paging.h"
#include <efi.h>

#define ARCHWAY_KERNEL_BASE UINT64_C(0xffffffff80000000)
#define ARCHWAY_KERNEL_MAX_SIZE UINT64_C(0x10000000)
struct kernel_image {
    efi_physical_address physical;
    size_t pages;
    uint64_t entry;
    struct page_map mapping;
};

efi_status
load_kernel(efi_handle image, struct efi_system_table *table, struct kernel_image *kernel);
void unload_kernel(struct efi_boot_services *bs, struct kernel_image *kernel);
#endif
