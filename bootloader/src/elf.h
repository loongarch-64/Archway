/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file elf.h
 * @brief ELF loader inf.
 */

#ifndef ARCHWAY_ELF_H
#define ARCHWAY_ELF_H
#include <efi.h>

#define ELF_MAGIC "\177ELF"
#define ELF_MAGIC_SIZE (sizeof(ELF_MAGIC) - 1)
#define ELF_EI_NIDENT 16
#define ELF_EI_CLASS 4
#define ELF_EI_DATA 5
#define ELF_EI_VERSION 6
#define ELF_CLASS_64 2
#define ELF_DATA_LSB 1
#define ELF_EV_CURRENT 1
#define ELF_ET_EXEC 2
#define ELF_MACHINE_LOONGARCH64 258
#define ELF64_HEADER_SIZE 64
#define ELF64_PROGRAM_HEADER_SIZE 56
#define ELF64_SECTION_HEADER_SIZE 64
#define ELF_PN_XNUM 0xffff
#define ELF_PF_X 1U
#define ELF_PF_W 2U
#define ELF_PF_R 4U
#define ELF_PT_LOAD 1U
#define ELF_PT_DYNAMIC 2U
#define ELF_PT_INTERP 3U
#define ELF_SHF_ALLOC (1U << 1)
#define ELF_SHT_NOBITS 8U

struct elf_load_policy {
    uint64_t virtual_base;
    uint64_t max_size;
    /* Nonzero requires aligned load segments and allocatable section headers. */
    uint64_t required_alignment;
    uint16_t machine;
    uint32_t allowed_segment_flags;
};

struct elf_segment {
    uint32_t type, flags;
    uint64_t offset, vaddr, paddr, filesz, memsz, align;
};

struct elf_image {
    uint64_t virtual_base, entry, memory_size;
    size_t segment_count;
    struct elf_segment *segments;
};

efi_status elf_inspect(
    struct efi_boot_services *bs,
    struct efi_file_protocol *file,
    const struct elf_load_policy *policy,
    struct elf_image *image
);
efi_status elf_load(
    struct efi_file_protocol *file,
    void *destination,
    size_t capacity,
    const struct elf_image *image
);
void elf_release(struct efi_boot_services *bs, struct elf_image *image);
#endif
