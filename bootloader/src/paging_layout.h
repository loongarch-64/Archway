/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file paging_layout.h
 * @brief Paging constants.
 */

#ifndef ARCHWAY_PAGING_LAYOUT_H
#define ARCHWAY_PAGING_LAYOUT_H

#include <archway/memory_layout.h>

#define PHYSICAL_BITS 48
#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PT_INDEX_BITS 9
#define PT_ENTRY_SHIFT 3
#define PT_ENTRY_SIZE (1 << PT_ENTRY_SHIFT)
#define PT_L1_SHIFT (PAGE_SHIFT + PT_INDEX_BITS)
#define PT_L2_SHIFT (PT_L1_SHIFT + PT_INDEX_BITS)
#define PT_L3_SHIFT (PT_L2_SHIFT + PT_INDEX_BITS)
#define TLB_PAIR_SHIFT (PAGE_SHIFT + 1)
#define TLB_PAIR_ENTRY_SHIFT (PT_ENTRY_SHIFT + 1)

#endif
