/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file mm.c
 * @brief Memory manager.
 */

#include <mem/mm.h>
#include <log.h>
#include <arch/hcf.h>

static const char *
memory_type_name(uint32_t type) {
    switch (type) {
    case ARCHWAY_MEMORY_USABLE: return "usable";
    case ARCHWAY_MEMORY_BOOTLOADER_RECLAIMABLE: return "bootloader-reclaimable";
    case ARCHWAY_MEMORY_ACPI_RECLAIMABLE: return "acpi-reclaimable";
    case ARCHWAY_MEMORY_ACPI_NVS: return "acpi-nvs";
    case ARCHWAY_MEMORY_MMIO: return "mmio";
    case ARCHWAY_MEMORY_RESERVED: return "reserved";
    default: return "unknown";
    }
}

static void
print_size(uint64_t bytes) {
    static const char *const units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    uint64_t divisor = 1;
    unsigned unit = 0;
    while (unit + 1 < sizeof(units) / sizeof(units[0]) && bytes / divisor >= 1024) {
        divisor *= 1024;
        ++unit;
    }

    if (unit == 0) {
        unsigned digits = 1;
        for (uint64_t value = bytes; value >= 10; value /= 10) {
            ++digits;
        }
        for (unsigned i = digits + 2; i < 9; ++i) {
            kprintf(" ");
        }
        kprintf("%llu B", (unsigned long long)bytes);
    } else {
        uint64_t whole = bytes / divisor;
        uint64_t fraction = (bytes % divisor) * 100 / divisor;
        unsigned digits = whole >= 100 ? 3 : whole >= 10 ? 2 : 1;
        for (unsigned i = digits + 6; i < 9; ++i) {
            kprintf(" ");
        }
        kprintf("%llu.", (unsigned long long)whole);
        if (fraction < 10) {
            kprintf("0");
        }
        kprintf("%llu %s", (unsigned long long)fraction, units[unit]);
    }
}

void
mm_print_memory_map(const struct archway_boot_info *boot_info) {
    if (boot_info == 0 || boot_info->memory_regions == 0) {
        kprintf("mm: unavailable\n");
        hcf();
    }

    kprintf("mm: memory map:\n");
    for (uint64_t i = 0; i < boot_info->memory_region_count; ++i) {
        const struct archway_memory_region *region = &boot_info->memory_regions[i];
        kprintf(
            "  base=0x%016llx  length=0x%016llx  size=",
            (unsigned long long)region->base,
            (unsigned long long)region->length
        );
        print_size(region->length);
        kprintf("  type=%s\n", memory_type_name(region->type));
    }
}
