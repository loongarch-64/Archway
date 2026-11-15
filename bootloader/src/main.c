/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file main.c
 * @brief Bootloader's entry point.
 */

#include <efi.h>

efi_status
efi_main(efi_handle image_handle, struct efi_system_table *system_table) {
    (void)image_handle;

    system_table->con_out->output_string(
        system_table->con_out,
        L"Hheelllloo..\r\n"
    );

    for (;;);

    return EFI_SUCCESS;
}
