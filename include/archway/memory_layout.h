/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file memory_layout.h
 * @brief Shared constants for memory.
 */

#ifndef ARCHWAY_MEMORY_LAYOUT_H
#define ARCHWAY_MEMORY_LAYOUT_H

#define ARCHWAY_HHDM_BASE 0x9000000000000000 /* cached, coherent */
#define ARCHWAY_MMIO_BASE 0x8000000000000000 /* uncached, strongly ordered */

#define ARCHWAY_VMALLOC_BASE 0xffff800000000000
#define ARCHWAY_VMALLOC_END 0xffffffff80000000 /* exclusive */

#define ARCHWAY_DRIVERS_BASE 0xffffffff90000000
#define ARCHWAY_DRIVERS_SIZE 0x70000000

#endif
