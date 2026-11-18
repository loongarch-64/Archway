/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file heap.h
 * @brief A header for size class slab allocator.
 */

#ifndef ARCHWAY_HEAP_H
#define ARCHWAY_HEAP_H

#include <stddef.h>

void *kmalloc(size_t size); /* returns a pointer in VMALLOC region. */
void kfree(void *pointer);  /* does nothing on NULL */

#endif
