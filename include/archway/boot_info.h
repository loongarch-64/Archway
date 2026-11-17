/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file boot_info.h
 * @brief Boot information from bootloader.
 */

#ifndef ARCHWAY_BOOT_INFO_H
#define ARCHWAY_BOOT_INFO_H

#include <stdint.h>

#define ARCHWAY_BOOT_INFO_MAGIC UINT64_C(0x4152434857415942)
#define ARCHWAY_BOOT_INFO_VERSION 1

enum archway_memory_region_type {
    ARCHWAY_MEMORY_USABLE = 1,
    ARCHWAY_MEMORY_BOOTLOADER_RECLAIMABLE,
    ARCHWAY_MEMORY_ACPI_RECLAIMABLE,
    ARCHWAY_MEMORY_ACPI_NVS,
    ARCHWAY_MEMORY_MMIO,
    ARCHWAY_MEMORY_RESERVED,
};

struct archway_memory_region {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
};

_Static_assert(sizeof(struct archway_memory_region) == 24, "unexpected memory-region size");

struct archway_boot_info {
    uint64_t magic;
    uint32_t version;
    uint32_t size;

    uint64_t acpi_rsdp; /* virtual address of acpi if available. */

    const struct archway_memory_region *memory_regions;
    uint64_t memory_region_count;
};

#endif
