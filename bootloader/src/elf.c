/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file elf.c
 * @brief ELF validation and segs loading.
 */

#include "elf.h"
#include "string.h"

struct elf_header {
    uint8_t ident[ELF_EI_NIDENT];
    uint16_t type, machine;
    uint32_t version;
    uint64_t entry, phoff, shoff;
    uint32_t flags;
    uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
};

struct elf_section {
    uint32_t name, type;
    uint64_t flags, address, offset, size;
    uint32_t link, info;
    uint64_t alignment, entry_size;
};

_Static_assert(sizeof(struct elf_header) == ELF64_HEADER_SIZE, "ELF64 header size");
_Static_assert(
    sizeof(struct elf_segment) == ELF64_PROGRAM_HEADER_SIZE, "ELF64 program header size"
);
_Static_assert(
    sizeof(struct elf_section) == ELF64_SECTION_HEADER_SIZE, "ELF64 section header size"
);

static efi_status
read_at(struct efi_file_protocol *file, uint64_t offset, void *buffer, size_t size) {
    efi_status status = file->set_position(file, offset);
    if (EFI_ERROR(status)) {
        return status;
    }
    while (size != 0) {
        size_t count = size;
        status = file->read(file, &count, buffer);
        if (EFI_ERROR(status)) {
            return status;
        }
        if (count == 0 || count > size) {
            return EFI_LOAD_ERROR;
        }
        buffer = (uint8_t *)buffer + count;
        size -= count;
    }
    return EFI_SUCCESS;
}

static efi_status
validate_sections(
    struct efi_file_protocol *file,
    const struct elf_header *header,
    uint64_t file_size,
    const struct elf_load_policy *policy,
    const struct elf_image *image
) {
    uint64_t mask = policy->required_alignment - 1;
    bool allocated_section = false;

    if (header->shnum == 0 || header->shentsize != sizeof(struct elf_section)
        || header->shoff < sizeof(*header) || header->shoff > file_size
        || header->shnum * sizeof(struct elf_section) > file_size - header->shoff
        || header->shstrndx >= header->shnum) {
        return EFI_LOAD_ERROR;
    }
    for (size_t i = 0; i < header->shnum; ++i) {
        struct elf_section section;
        efi_status status =
            read_at(file, header->shoff + i * sizeof(section), &section, sizeof(section));
        if (EFI_ERROR(status)) {
            return status;
        }
        if ((section.flags & ELF_SHF_ALLOC) == 0) {
            continue;
        }
        if (((section.address | section.size) & mask) != 0
            || section.alignment < policy->required_alignment
            || (section.alignment & (section.alignment - 1)) != 0
            || (section.address & (section.alignment - 1)) != 0
            || section.address < policy->virtual_base
            || section.address - policy->virtual_base >= policy->max_size
            || section.size > policy->max_size - (section.address - policy->virtual_base)) {
            return EFI_LOAD_ERROR;
        }
        if (section.type != ELF_SHT_NOBITS
            && ((section.offset & mask) != 0 || section.offset > file_size
                || section.size > file_size - section.offset)) {
            return EFI_LOAD_ERROR;
        }
        if (section.size == 0) {
            continue;
        }
        bool contained = false;
        for (size_t j = 0; j < image->segment_count; ++j) {
            const struct elf_segment *p = &image->segments[j];
            if (p->type != ELF_PT_LOAD || section.address < p->vaddr) {
                continue;
            }
            uint64_t offset = section.address - p->vaddr;
            if (offset > p->memsz || section.size > p->memsz - offset) {
                continue;
            }
            if (section.type != ELF_SHT_NOBITS
                && (offset > p->filesz || section.size > p->filesz - offset
                    || section.offset != p->offset + offset)) {
                continue;
            }
            contained = true;
            break;
        }
        if (!contained) {
            return EFI_LOAD_ERROR;
        }
        allocated_section = true;
    }
    return allocated_section ? EFI_SUCCESS : EFI_LOAD_ERROR;
}

void
elf_release(struct efi_boot_services *bs, struct elf_image *image) {
    if (image->segments != NULL) {
        bs->free_pool(image->segments);
    }
    memset(image, 0, sizeof(*image));
}

efi_status
elf_inspect(
    struct efi_boot_services *bs,
    struct efi_file_protocol *file,
    const struct elf_load_policy *policy,
    struct elf_image *image
) {
    struct elf_header header;
    uint64_t file_size, memory_size = 0;
    bool entry_found = false;
    efi_status status;

    if (bs == NULL || file == NULL || policy == NULL || image == NULL || policy->max_size == 0
        || policy->max_size - 1 > UINT64_MAX - policy->virtual_base
        || (policy->required_alignment != 0
            && (policy->required_alignment & (policy->required_alignment - 1)) != 0)) {
        return EFI_INVALID_PARAMETER;
    }
    memset(image, 0, sizeof(*image));
    status = file->set_position(file, UINT64_MAX);
    if (EFI_ERROR(status)) {
        return status;
    }
    status = file->get_position(file, &file_size);
    if (EFI_ERROR(status)) {
        return status;
    }
    status = read_at(file, 0, &header, sizeof(header));
    if (EFI_ERROR(status)) {
        return status;
    }
    if (memcmp(header.ident, ELF_MAGIC, ELF_MAGIC_SIZE) != 0
        || header.ident[ELF_EI_CLASS] != ELF_CLASS_64 || header.ident[ELF_EI_DATA] != ELF_DATA_LSB
        || header.ident[ELF_EI_VERSION] != ELF_EV_CURRENT || header.type != ELF_ET_EXEC
        || header.machine != policy->machine || header.version != ELF_EV_CURRENT
        || header.ehsize != sizeof(header) || header.phentsize != sizeof(struct elf_segment)
        || header.phnum == 0 || header.phnum == ELF_PN_XNUM || header.phoff < sizeof(header)
        || header.phoff > file_size
        || header.phnum * sizeof(struct elf_segment) > file_size - header.phoff) {
        return EFI_LOAD_ERROR;
    }
    status = bs->allocate_pool(
        EFI_LOADER_DATA, header.phnum * sizeof(struct elf_segment), (void **)&image->segments
    );
    if (EFI_ERROR(status)) {
        return status;
    }
    image->segment_count = header.phnum;
    status =
        read_at(file, header.phoff, image->segments, header.phnum * sizeof(struct elf_segment));
    if (EFI_ERROR(status)) {
        goto fail;
    }
    status = EFI_LOAD_ERROR;
    for (size_t i = 0; i < image->segment_count; ++i) {
        const struct elf_segment *p = &image->segments[i];
        if (p->type == ELF_PT_DYNAMIC || p->type == ELF_PT_INTERP) {
            goto fail;
        }
        if (p->type != ELF_PT_LOAD) {
            continue;
        }
        if (policy->required_alignment != 0
            && (p->align < policy->required_alignment
                || ((p->vaddr | p->offset | p->filesz | p->memsz)
                    & (policy->required_alignment - 1))
                       != 0)) {
            goto fail;
        }
        if (p->memsz == 0) {
            if (p->filesz != 0) {
                goto fail;
            }
            continue;
        }
        if (p->vaddr < policy->virtual_base || p->vaddr - policy->virtual_base >= policy->max_size
            || p->memsz > policy->max_size - (p->vaddr - policy->virtual_base)
            || p->filesz > p->memsz || p->offset > file_size || p->filesz > file_size - p->offset
            || (p->flags & ~policy->allowed_segment_flags) != 0
            || (p->align > 1
                && ((p->align & (p->align - 1)) != 0
                    || (p->vaddr & (p->align - 1)) != (p->offset & (p->align - 1))))) {
            goto fail;
        }
        uint64_t begin = p->vaddr - policy->virtual_base;
        uint64_t end = begin + p->memsz;
        for (size_t j = 0; j < i; ++j) {
            const struct elf_segment *other = &image->segments[j];
            uint64_t other_begin = other->vaddr - policy->virtual_base;
            if (other->type == ELF_PT_LOAD && other->memsz != 0
                && begin < other_begin + other->memsz && other_begin < end) {
                goto fail;
            }
        }
        if (end > memory_size) {
            memory_size = end;
        }
        if ((p->flags & ELF_PF_X) != 0 && header.entry >= p->vaddr
            && header.entry - p->vaddr < p->filesz) {
            entry_found = true;
        }
    }
    if (!entry_found) {
        goto fail;
    }
    if (policy->required_alignment != 0) {
        status = validate_sections(file, &header, file_size, policy, image);
        if (EFI_ERROR(status)) {
            goto fail;
        }
    }
    image->virtual_base = policy->virtual_base;
    image->entry = header.entry;
    image->memory_size = memory_size;
    return EFI_SUCCESS;
fail:
    elf_release(bs, image);
    return status;
}

efi_status
elf_load(
    struct efi_file_protocol *file,
    void *destination,
    size_t capacity,
    const struct elf_image *image
) {
    if (file == NULL || destination == NULL || image == NULL || image->segments == NULL
        || image->memory_size > capacity) {
        return EFI_INVALID_PARAMETER;
    }
    memset(destination, 0, capacity);
    for (size_t i = 0; i < image->segment_count; ++i) {
        const struct elf_segment *p = &image->segments[i];
        if (p->type != ELF_PT_LOAD || p->filesz == 0) {
            continue;
        }
        efi_status status = read_at(
            file, p->offset, (uint8_t *)destination + (p->vaddr - image->virtual_base), p->filesz
        );
        if (EFI_ERROR(status)) {
            return status;
        }
    }
    return EFI_SUCCESS;
}
