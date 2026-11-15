/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file mm.c
 * @brief Memory manager functions.
 */

#include "mm.h"
#include "strhlp.h"
#include "string.h"

static const char *
memory_type_name(uint32_t type) {
    switch (type) {
    case EFI_RESERVED_MEMORY_TYPE: return "Reserved";
    case EFI_LOADER_CODE: return "LoaderCode";
    case EFI_LOADER_DATA: return "LoaderData";
    case EFI_BOOT_SERVICES_CODE: return "BootCode";
    case EFI_BOOT_SERVICES_DATA: return "BootData";
    case EFI_RUNTIME_SERVICES_CODE: return "RuntimeCode";
    case EFI_RUNTIME_SERVICES_DATA: return "RuntimeData";
    case EFI_CONVENTIONAL_MEMORY: return "Conventional";
    case EFI_UNUSABLE_MEMORY: return "Unusable";
    case EFI_ACPI_RECLAIM_MEMORY: return "ACPIReclaim";
    case EFI_ACPI_MEMORY_NVS: return "ACPINVS";
    case EFI_MEMORY_MAPPED_IO: return "MMIO";
    case EFI_MEMORY_MAPPED_IO_PORT_SPACE: return "MMIOPort";
    case EFI_PAL_CODE: return "PALCode";
    case EFI_PERSISTENT_MEMORY: return "Persistent";
    case EFI_UNACCEPTED_MEMORY_TYPE: return "Unaccepted";
    default: return "Unknown";
    }
}

static efi_status
output_line(struct efi_system_table *system_table, uint16_t *line, size_t length) {
    line[length] = 0;
    return system_table->con_out->output_string(system_table->con_out, line);
}

static efi_status
output_status(struct efi_system_table *system_table, const char *message, efi_status status) {
    uint16_t line[96];
    size_t length = 0;

    if (!append_ascii(line, sizeof(line) / sizeof(line[0]), &length, message)
        || !append_ascii(line, sizeof(line) / sizeof(line[0]), &length, " status=0x")
        || !append_hex(line, sizeof(line) / sizeof(line[0]), &length, (uint64_t)status, 16)
        || !append_ascii(line, sizeof(line) / sizeof(line[0]), &length, "\r\n")) {
        return EFI_BUFFER_TOO_SMALL;
    }
    return output_line(system_table, line, length);
}

static efi_status
output_descriptor(
    struct efi_system_table *system_table, const struct efi_memory_descriptor *descriptor
) {
    uint16_t line[128];
    size_t length = 0;

    if (!append_ascii(
            line, sizeof(line) / sizeof(line[0]), &length, memory_type_name(descriptor->type)
        )
        || !append_ascii(line, sizeof(line) / sizeof(line[0]), &length, " start=0x")
        || !append_hex(
            line, sizeof(line) / sizeof(line[0]), &length, descriptor->physical_start, 16
        )
        || !append_ascii(line, sizeof(line) / sizeof(line[0]), &length, " pages=")
        || !append_decimal(
            line, sizeof(line) / sizeof(line[0]), &length, descriptor->number_of_pages
        )
        || !append_ascii(line, sizeof(line) / sizeof(line[0]), &length, " attr=0x")
        || !append_hex(line, sizeof(line) / sizeof(line[0]), &length, descriptor->attribute, 16)
        || !append_ascii(line, sizeof(line) / sizeof(line[0]), &length, "\r\n")) {
        return EFI_BUFFER_TOO_SMALL;
    }
    return output_line(system_table, line, length);
}

efi_status
print_memory_map(struct efi_system_table *system_table) {
    struct efi_memory_descriptor *memory_map = NULL;
    size_t memory_map_size = 0;
    size_t map_key = 0;
    size_t descriptor_size = 0;
    size_t descriptor_count = 0;
    uint32_t descriptor_version = 0;
    efi_status status;

    if (system_table == NULL || system_table->boot_services == NULL
        || system_table->con_out == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    status = system_table->boot_services->get_memory_map(
        &memory_map_size, NULL, &map_key, &descriptor_size, &descriptor_version
    );
    if (status != EFI_BUFFER_TOO_SMALL) {
        output_status(system_table, "GetMemoryMap sizing failed", status);
        return status;
    }

    for (size_t attempt = 0; attempt < 4; ++attempt) {
        size_t slack;
        size_t buffer_size;
        void *buffer = NULL;

        if (descriptor_size == 0) {
            descriptor_size = sizeof(*memory_map);
        }
        if (descriptor_size < sizeof(*memory_map) || descriptor_size > ((size_t)-1) / 8) {
            output_status(system_table, "Invalid memory-map descriptor size", EFI_LOAD_ERROR);
            return EFI_LOAD_ERROR;
        }

        slack = descriptor_size * 8;
        if (memory_map_size > (size_t)-1 - slack) {
            output_status(system_table, "Memory-map buffer size overflow", EFI_OUT_OF_RESOURCES);
            return EFI_OUT_OF_RESOURCES;
        }
        buffer_size = memory_map_size + slack;

        status = system_table->boot_services->allocate_pool(EFI_LOADER_DATA, buffer_size, &buffer);
        if (EFI_ERROR(status)) {
            output_status(system_table, "Memory-map allocation failed", status);
            return status;
        }

        memory_map = buffer;
        memory_map_size = buffer_size;
        status = system_table->boot_services->get_memory_map(
            &memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version
        );
        if (status != EFI_BUFFER_TOO_SMALL) {
            break;
        }

        system_table->boot_services->free_pool(memory_map);
        memory_map = NULL;
    }

    if (status != EFI_SUCCESS) {
        output_status(system_table, "GetMemoryMap failed", status);
        if (memory_map != NULL) {
            system_table->boot_services->free_pool(memory_map);
        }
        return status;
    }

    if (descriptor_size < sizeof(*memory_map) || descriptor_size == 0
        || memory_map_size < descriptor_size || memory_map_size % descriptor_size != 0) {
        output_status(system_table, "Invalid memory-map result", EFI_LOAD_ERROR);
        system_table->boot_services->free_pool(memory_map);
        return EFI_LOAD_ERROR;
    }
    descriptor_count = memory_map_size / descriptor_size;

    {
        uint16_t line[96];
        size_t length = 0;

        if (!append_ascii(line, sizeof(line) / sizeof(line[0]), &length, "UEFI memory map: ")
            || !append_decimal(line, sizeof(line) / sizeof(line[0]), &length, descriptor_count)
            || !append_ascii(line, sizeof(line) / sizeof(line[0]), &length, " descriptors, ")
            || !append_decimal(line, sizeof(line) / sizeof(line[0]), &length, descriptor_size)
            || !append_ascii(line, sizeof(line) / sizeof(line[0]), &length, " bytes each, version ")
            || !append_decimal(line, sizeof(line) / sizeof(line[0]), &length, descriptor_version)
            || !append_ascii(line, sizeof(line) / sizeof(line[0]), &length, "\r\n")) {
            system_table->boot_services->free_pool(memory_map);
            return EFI_BUFFER_TOO_SMALL;
        }
        status = output_line(system_table, line, length);
        if (EFI_ERROR(status)) {
            system_table->boot_services->free_pool(memory_map);
            return status;
        }
    }

    for (size_t i = 0; i < descriptor_count; ++i) {
        struct efi_memory_descriptor descriptor;
        const uint8_t *entry = (const uint8_t *)memory_map + i * descriptor_size;

        memcpy(&descriptor, entry, sizeof(descriptor));
        status = output_descriptor(system_table, &descriptor);
        if (EFI_ERROR(status)) {
            system_table->boot_services->free_pool(memory_map);
            return status;
        }
    }

    system_table->boot_services->free_pool(memory_map);
    return EFI_SUCCESS;
}
