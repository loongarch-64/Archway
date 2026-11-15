/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file handoff.h
 * @brief UEFI boot services exit and kernel handoff.
 */

#ifndef ARCHWAY_HANDOFF_H
#define ARCHWAY_HANDOFF_H
#include <efi.h>

efi_status boot_kernel(efi_handle image, struct efi_system_table *table);
#endif
