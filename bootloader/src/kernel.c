/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file kernel.c
 * @brief Kernel loading.
 */

#include "kernel.h"
#include "elf.h"
#include "string.h"

efi_status
load_kernel(efi_handle image, struct efi_system_table *table, struct kernel_image *kernel) {
    struct efi_boot_services *bs = table->boot_services;
    struct efi_guid image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    struct efi_guid fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    struct efi_loaded_image_protocol *loaded;
    struct efi_simple_file_system_protocol *fs;
    struct efi_file_protocol *root = NULL, *file = NULL;
    const struct elf_load_policy policy = {
        .virtual_base = ARCHWAY_KERNEL_BASE,
        .max_size = UINT64_MAX - ARCHWAY_KERNEL_BASE + 1,
        .required_alignment = EFI_PAGE_SIZE,
        .machine = ELF_MACHINE_LOONGARCH64,
        .allowed_segment_flags = ELF_PF_R | ELF_PF_W | ELF_PF_X,
    };
    struct elf_image elf = {0};
    efi_status status;

    memset(kernel, 0, sizeof(*kernel));
    status = bs->handle_protocol(image, &image_guid, (void **)&loaded);
    if (EFI_ERROR(status)) {
        return status;
    }
    status = bs->handle_protocol(loaded->device_handle, &fs_guid, (void **)&fs);
    if (EFI_ERROR(status)) {
        return status;
    }
    status = fs->open_volume(fs, &root);
    if (EFI_ERROR(status)) {
        return status;
    }
    status = root->open(root, &file, L"\\archway.elf", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        goto done;
    }
    status = elf_inspect(bs, file, &policy, &elf);
    if (EFI_ERROR(status)) {
        goto done;
    }
    kernel->pages = (elf.memory_size + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE;
    status = bs->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_CODE, kernel->pages, &kernel->physical
    );
    if (EFI_ERROR(status)) {
        kernel->pages = 0;
        goto done;
    }
    status =
        elf_load(file, (void *)(uintptr_t)kernel->physical, kernel->pages * EFI_PAGE_SIZE, &elf);
    if (EFI_ERROR(status)) {
        goto done;
    }
    status = page_map_init(bs, &kernel->mapping);
    if (EFI_ERROR(status)) {
        goto done;
    }
    for (size_t i = 0; i < elf.segment_count; ++i) {
        const struct elf_segment *p = &elf.segments[i];
        if (p->type != ELF_PT_LOAD || p->memsz == 0) {
            continue;
        }
        uint64_t start = p->vaddr & ~UINT64_C(0xfff);
        size_t size = ((p->vaddr & 0xfff) + p->memsz + 0xfff) & ~UINT64_C(0xfff);
        status = page_map_range(
            bs,
            &kernel->mapping,
            start,
            kernel->physical + (start - ARCHWAY_KERNEL_BASE),
            size,
            p->flags
        );
        if (EFI_ERROR(status)) {
            goto done;
        }
    }
    status = bs->allocate_pages(
        EFI_ALLOCATE_ANY_PAGES, EFI_LOADER_DATA, ARCHWAY_KERNEL_STACK_PAGES, &kernel->stack
    );
    if (EFI_ERROR(status)) {
        goto done;
    }
    memset((void *)(uintptr_t)kernel->stack, 0, ARCHWAY_KERNEL_STACK_PAGES * EFI_PAGE_SIZE);
    kernel->entry = elf.entry;
done:
    if (file != NULL) {
        efi_status close_status = file->close(file);
        if (!EFI_ERROR(status)) {
            status = close_status;
        }
    }
    {
        efi_status close_status = root->close(root);
        if (!EFI_ERROR(status)) {
            status = close_status;
        }
    }
    elf_release(bs, &elf);
    if (EFI_ERROR(status)) {
        unload_kernel(bs, kernel);
    }
    return status;
}

void
unload_kernel(struct efi_boot_services *bs, struct kernel_image *kernel) {
    page_map_destroy(bs, &kernel->mapping);
    if (kernel->stack != 0) {
        bs->free_pages(kernel->stack, ARCHWAY_KERNEL_STACK_PAGES);
    }
    if (kernel->pages != 0) {
        bs->free_pages(kernel->physical, kernel->pages);
    }
    memset(kernel, 0, sizeof(*kernel));
}
