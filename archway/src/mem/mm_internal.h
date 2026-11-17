/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file mm_internal.h
 * @brief Internal memory manager structures and definitions.
 */

#ifndef ARCHWAY_MM_INTERNAL_H
#define ARCHWAY_MM_INTERNAL_H

#include <stdint.h>

struct free_range {
    uint64_t base;
    uint64_t pages;
    struct free_range *next;
};

#endif
