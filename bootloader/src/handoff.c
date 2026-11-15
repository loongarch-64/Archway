/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file handoff.c
 * @brief UEFI boot services exit and kernel handoff.
 */

#include "handoff.h"
#include "kernel.h"

_Noreturn void enter_kernel(efi_physical_address root, uint64_t entry, uint64_t stack);

efi_status
boot_kernel(efi_handle image, struct efi_system_table *table) {
    struct efi_boot_services *bs = table->boot_services;
    struct efi_memory_descriptor *map = NULL;
    size_t capacity = 0, size = 0, key = 0, stride = 0;
    uint32_t version = 0;
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

    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        status = bs->exit_boot_services(image, key);
        if (status == EFI_SUCCESS) {
            enter_kernel(
                kernel.mapping.root,
                kernel.entry,
                ARCHWAY_HHDM_BASE + kernel.stack + ARCHWAY_KERNEL_STACK_PAGES * EFI_PAGE_SIZE
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
    }

    __asm__ volatile("csrwr $zero, 0x4" ::: "memory");
    for (;;) {
        __asm__ volatile("nop");
    }

fail:
    if (map != NULL) {
        bs->free_pool(map);
    }
    unload_kernel(bs, &kernel);
    return status;
}
