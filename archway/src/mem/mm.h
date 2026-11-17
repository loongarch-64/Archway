/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file mm.h
 * @brief Memory manager.
 */

#ifndef ARCHWAY_MM_H
#define ARCHWAY_MM_H

#include <archway/boot_info.h>

#define MM_PAGE_SIZE UINT64_C(4096)

void mm_init(const struct archway_boot_info *boot_info);
uint64_t alloc_page(void);            /* allocates a physical page */
void dealloc_page(uint64_t physical); /* deallocates a physical page. NULL is no-op */

void mm_print_memory_map(const struct archway_boot_info *boot_info);

#endif
