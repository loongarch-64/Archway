/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file paging.h
 * @brief LoongArch page-table interface.
 */

#ifndef ARCHWAY_ARCH_PAGING_H
#define ARCHWAY_ARCH_PAGING_H

#include <stdint.h>

enum paging_flags {
    PAGING_READ = 1u << 0,
    PAGING_WRITE = 1u << 1,
    PAGING_EXECUTE = 1u << 2,
    PAGING_UNCACHED = 1u << 3,
};

enum paging_result {
    PAGING_OK = 0,
    PAGING_INVALID_ARGUMENT,
    PAGING_NO_MEMORY,   /* ran out of physical pages to create mappings */
    PAGING_CONFLICT,    /* there's something mapped already */
    PAGING_UNSUPPORTED, /* encountered an entry format this page-table walker cannot handle */
};

/* installs the kernel refill handler using 
   the physical load base. 
   
   PGDL is allocated on demand. */
enum paging_result paging_init(uint64_t kernel_physical_base);

/* returns the active PGDL or PGDH root 
   selected by the VA's VALEN-1 bit. */
uint64_t paging_root_physical(uint64_t virtual_address);

enum paging_result
paging_map_page(uint64_t virtual_address, uint64_t physical_address, enum paging_flags flags);
/* on success it may optionally return 
   the physical address of the unmapped page */
enum paging_result paging_unmap_page(uint64_t virtual_address, uint64_t *physical_address);

#endif /* ARCHWAY_ARCH_PAGING_H */
