/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file mm.h
 * @brief Memory manager.
 */

#ifndef ARCHWAY_MM_H
#define ARCHWAY_MM_H

#include <archway/boot_info.h>

void mm_print_memory_map(const struct archway_boot_info *boot_info);

#endif
