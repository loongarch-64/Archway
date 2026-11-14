/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file main.c
 * @brief Bootloader's entry point.
 */

#include <Uefi.h>
#include <Library/UefiLib.h>

EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
) {
    (VOID)ImageHandle;
    (VOID)SystemTable;

    Print(L"Hheelllloo..\r\n");

    return EFI_SUCCESS;
}