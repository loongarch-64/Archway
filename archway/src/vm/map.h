/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file map.h
 * @brief Kernel virtual-memory mapping interface.
 */

#ifndef ARCHWAY_VM_MAP_H
#define ARCHWAY_VM_MAP_H

#include <stddef.h>
#include <stdint.h>

enum vm_map_flags {
    VM_MAP_READ = 1u << 8,
    VM_MAP_WRITE = 1u << 9,
    VM_MAP_EXECUTE = 1u << 10,
    VM_MAP_UNCACHED = 1u << 11,
};

enum vm_map_result {
    VM_MAP_OK = 0,
    VM_MAP_INVALID_ARGUMENT,
    VM_MAP_NO_MEMORY,
    VM_MAP_CONFLICT,
    VM_MAP_UNSUPPORTED,
};

enum vm_map_result vm_map_range(
    uint64_t virtual_address, uint64_t physical_address, size_t size, enum vm_map_flags flags
);

/* on success it may optionally return 
   the physical address of the unmapped page */
enum vm_map_result vm_unmap_page(uint64_t virtual_address, uint64_t *physical_address);
enum vm_map_result vm_unmap_range(uint64_t virtual_address, size_t size);

#endif /* ARCHWAY_VM_MAP_H */
