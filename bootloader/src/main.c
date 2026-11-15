/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file main.c
 * @brief Bootloader's entry point.
 */

#include <efi.h>

#include "mm.h"

efi_status
efi_main(efi_handle image_handle, struct efi_system_table *system_table) {
    (void)image_handle;
    efi_status status;

    status = system_table->boot_services->set_watchdog_timer(0, 0, 0, NULL);
    if (EFI_ERROR(status)) {
        return status;
    }

    status = system_table->con_out->output_string(
        system_table->con_out,
        L"Hheelllloo..\r\n"
    );

    if (EFI_ERROR(status)) {
        return status;
    }

    status = print_memory_map(system_table);
    if (EFI_ERROR(status)) {
        return status;
    }

    for (;;);

    return EFI_SUCCESS;
}
