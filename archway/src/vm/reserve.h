/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file reserve.h
 * @brief Kernel virtual address allocation.
 */

#ifndef ARCHWAY_VM_RESERVE_H
#define ARCHWAY_VM_RESERVE_H

#include <stdbool.h>
#include <stddef.h>

enum vm_region {
    VM_REGION_VMALLOC,
    VM_REGION_DRIVERS,
};

void *vm_reserve(enum vm_region region, size_t length);
bool vm_release(void *base, size_t length);

#endif /* ARCHWAY_VM_RESERVE_H */
