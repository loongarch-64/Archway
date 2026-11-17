/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file handoff.c
 * @brief UEFI boot services exit and kernel handoff.
 */

#include "handoff.h"
#include "kernel.h"
#include "string.h"
#include <archway/boot_info.h>
#include <archway/memory_layout.h>
#include <stdbool.h>

_Noreturn void
enter_kernel(efi_physical_address root, uint64_t entry, const struct archway_boot_info *boot_info);

static uint32_t
archway_memory_type(uint32_t type) {
    switch (type) {
    case EFI_CONVENTIONAL_MEMORY: return ARCHWAY_MEMORY_USABLE;
    case EFI_BOOT_SERVICES_CODE:
    case EFI_BOOT_SERVICES_DATA: return ARCHWAY_MEMORY_BOOTLOADER_RECLAIMABLE;
    case EFI_ACPI_RECLAIM_MEMORY: return ARCHWAY_MEMORY_ACPI_RECLAIMABLE;
    case EFI_ACPI_MEMORY_NVS: return ARCHWAY_MEMORY_ACPI_NVS;
    case EFI_MEMORY_MAPPED_IO:
    case EFI_MEMORY_MAPPED_IO_PORT_SPACE: return ARCHWAY_MEMORY_MMIO;
    default: return ARCHWAY_MEMORY_RESERVED;
    }
}

static bool
normalize_memory_map(
    void *buffer, size_t size, size_t stride, uint32_t descriptor_version, uint64_t *region_count
) {
    if (buffer == NULL || stride < sizeof(struct efi_memory_descriptor) || size < stride
        || size % stride != 0 || descriptor_version != EFI_MEMORY_DESCRIPTOR_VERSION) {
        return false;
    }
    size_t count = size / stride;
    struct archway_memory_region *regions = buffer;
    const uint8_t *descriptors = buffer;
    size_t output_count = 0;

    for (size_t i = 0; i < count; ++i) {
        struct efi_memory_descriptor desc;
        memcpy(&desc, descriptors + i * stride, sizeof(desc));
        if ((desc.physical_start & (EFI_PAGE_SIZE - 1)) != 0 || desc.number_of_pages == 0
            || desc.number_of_pages > UINT64_MAX / EFI_PAGE_SIZE) {
            return false;
        }
        uint64_t length = desc.number_of_pages * EFI_PAGE_SIZE;
        if (desc.physical_start > UINT64_MAX - length) {
            return false;
        }
        regions[output_count++] = (struct archway_memory_region){
            .base = desc.physical_start,
            .length = length,
            .type = archway_memory_type(desc.type),
            .reserved = 0,
        };
    }

    for (size_t i = 1; i < output_count; ++i) {
        struct archway_memory_region value = regions[i];
        size_t j = i;
        while (j != 0 && regions[j - 1].base > value.base) {
            regions[j] = regions[j - 1];
            --j;
        }
        regions[j] = value;
    }

    size_t merged = 0;
    for (size_t i = 0; i < output_count; ++i) {
        struct archway_memory_region current = regions[i];
        if (merged != 0) {
            struct archway_memory_region *previous = &regions[merged - 1];
            uint64_t previous_end = previous->base + previous->length;
            if (current.base < previous_end) {
                return false;
            }
            if (current.base == previous_end && current.type == previous->type) {
                if (current.length > UINT64_MAX - previous->length) {
                    return false;
                }
                previous->length += current.length;
                continue;
            }
        }
        regions[merged++] = current;
    }
    *region_count = merged;
    return merged != 0;
}

static bool
guid_equal(const struct efi_guid *left, const struct efi_guid *right) {
    return memcmp(left, right, sizeof(*left)) == 0;
}

// todo: move me to separate source handling acpi handjobs

static efi_physical_address
find_acpi_rsdp(struct efi_system_table *table) {
    static const struct efi_guid acpi_20_guid = EFI_ACPI_20_TABLE_GUID;
    static const struct efi_guid acpi_10_guid = EFI_ACPI_10_TABLE_GUID;

    if (table->configuration_table == NULL) {
        return 0;
    }

    for (size_t pass = 0; pass < 2; ++pass) {
        const struct efi_guid *guid = pass == 0 ? &acpi_20_guid : &acpi_10_guid;

        for (size_t i = 0; i < table->number_of_table_entries; ++i) {
            const struct efi_configuration_table *entry = &table->configuration_table[i];
            if (entry->vendor_table == NULL) {
                continue;
            }
            if (!guid_equal(&entry->vendor_guid, guid)) {
                continue;
            }
            return (efi_physical_address)(uintptr_t)entry->vendor_table;
        }
    }

    return 0;
}

static uint64_t
to_hhdm(efi_physical_address physical) {
    return physical == 0 ? 0 : ARCHWAY_HHDM_BASE + physical;
}

efi_status
boot_kernel(efi_handle image, struct efi_system_table *table) {
    struct efi_boot_services *bs = table->boot_services;
    struct efi_memory_descriptor *map = NULL;
    size_t capacity = 0, size = 0, key = 0, stride = 0;
    uint64_t region_count = 0;
    uint32_t version = 0;
    struct archway_boot_info *boot_info = NULL;
    struct kernel_image kernel;
    efi_status status;

    status = load_kernel(image, table, &kernel);
    if (EFI_ERROR(status)) {
        return status;
    }
    /* synchronize code with insn fetching */
    __asm__ volatile("dbar 0\n\t"
                     "ibar 0" ::
                         : "memory");

    status = bs->allocate_pool(EFI_LOADER_DATA, sizeof(*boot_info), (void **)&boot_info);
    if (EFI_ERROR(status)) {
        goto fail;
    }
    memset(boot_info, 0, sizeof(*boot_info));
    boot_info->magic = ARCHWAY_BOOT_INFO_MAGIC;
    boot_info->version = ARCHWAY_BOOT_INFO_VERSION;
    boot_info->size = sizeof(*boot_info);
    boot_info->acpi_rsdp = to_hhdm(find_acpi_rsdp(table));
    boot_info->kernel_physical_base = kernel.physical;

    status = bs->get_memory_map(&size, NULL, &key, &stride, &version);
    if (status != EFI_BUFFER_TOO_SMALL) {
        status = EFI_LOAD_ERROR;
        goto fail;
    }
    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        if (stride < sizeof(*map) || stride > (size_t)-1 / 32 || size > (size_t)-1 - stride * 32) {
            status = EFI_LOAD_ERROR;
            goto fail;
        }
        capacity = size + stride * 32;
        status = bs->allocate_pool(EFI_LOADER_DATA, capacity, (void **)&map);
        if (EFI_ERROR(status)) {
            goto fail;
        }
        size = capacity;
        status = bs->get_memory_map(&size, map, &key, &stride, &version);
        if (status != EFI_BUFFER_TOO_SMALL) {
            break;
        }
        bs->free_pool(map);
        map = NULL;
    }
    if (status != EFI_SUCCESS) {
        goto fail;
    }

    if (!normalize_memory_map(map, size, stride, version, &region_count)) {
        status = EFI_LOAD_ERROR;
        goto fail;
    }
    boot_info->memory_regions = (const struct archway_memory_region *)(uintptr_t)to_hhdm(
        (efi_physical_address)(uintptr_t)map
    );
    boot_info->memory_region_count = region_count;

    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        status = bs->exit_boot_services(image, key);
        if (status == EFI_SUCCESS) {
            enter_kernel(
                kernel.mapping.root,
                kernel.entry,
                (const struct archway_boot_info *)(uintptr_t)
                    to_hhdm((efi_physical_address)(uintptr_t)boot_info)
            );
        }
        if (status != EFI_INVALID_PARAMETER) {
            break;
        }
        size = capacity;
        status = bs->get_memory_map(&size, map, &key, &stride, &version);
        if (status != EFI_SUCCESS) {
            break;
        }
        if (!normalize_memory_map(map, size, stride, version, &region_count)) {
            status = EFI_LOAD_ERROR;
            break;
        }
        boot_info->memory_regions = (const struct archway_memory_region *)(uintptr_t)to_hhdm(
            (efi_physical_address)(uintptr_t)map
        );
        boot_info->memory_region_count = region_count;
    }

    __asm__ volatile("csrwr $zero, 0x4" ::: "memory");
    for (;;) {
        __asm__ volatile("nop");
    }

fail:
    if (map != NULL) {
        bs->free_pool(map);
    }
    if (boot_info != NULL) {
        bs->free_pool(boot_info);
    }
    unload_kernel(bs, &kernel);
    return status;
}
