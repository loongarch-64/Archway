/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file mm.h
 * @brief Memory manager functions.
 */

#ifndef ARCHWAY_MM_H
#define ARCHWAY_MM_H

#include <efi.h>

efi_status print_memory_map(struct efi_system_table *system_table);

#endif
