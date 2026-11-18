/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file paging.c
 * @brief LoongArch paging.
 */

#include <arch/paging.h>
#include <archway/memory_layout.h>
#include <mem/mm.h>
#include <stdbool.h>
#include <stddef.h>
#include <sync/spinlock.h>

#define PAGE_SHIFT 12u
#define PAGE_SIZE (UINT64_C(1) << PAGE_SHIFT)
#define PAGE_MASK (PAGE_SIZE - 1)
#define PT_INDEX_MASK UINT64_C(0x1ff)
#define PHYSICAL_ADDRESS_MASK UINT64_C(0x0000fffffffff000)
#define PHYSICAL_ADDRESS_MAX (PHYSICAL_ADDRESS_MASK | PAGE_MASK)

#define PTE_VALID UINT64_C(0x001)
#define PTE_DIRTY UINT64_C(0x002)
#define PTE_MAT_COHERENT UINT64_C(0x010)
#define PTE_GLOBAL UINT64_C(0x040)
#define PDE_HUGE UINT64_C(0x040)
#define PDE_OWNED UINT64_C(0x800)
#define PTE_NO_READ (UINT64_C(1) << 61)
#define PTE_NO_EXEC (UINT64_C(1) << 62)

extern void paging_flush_tlb(void);
extern void paging_install_low_root(uint64_t physical_address);
extern void paging_install_refill(uint64_t physical_address);
extern char paging_refill_entry[];
extern char __kernel_start[];

/* Serializes C page-table writers. The assembly refill handler walks locklessly. */
static spinlock_t page_table_lock = SPINLOCK_INITIALIZER;
static bool low_root_owned;

static uint64_t *table_from_physical(uint64_t physical);

enum paging_result
paging_init(uint64_t kernel_physical_base) {
    uint64_t image_virtual = (uint64_t)(uintptr_t)__kernel_start;
    uint64_t refill_virtual = (uint64_t)(uintptr_t)paging_refill_entry;
    if (refill_virtual < image_virtual) {
        return PAGING_INVALID_ARGUMENT;
    }
    uint64_t refill_offset = refill_virtual - image_virtual;
    if (refill_offset > PHYSICAL_ADDRESS_MAX
        || kernel_physical_base > PHYSICAL_ADDRESS_MAX - refill_offset) {
        return PAGING_INVALID_ARGUMENT;
    }
    uint64_t refill_physical = kernel_physical_base + refill_offset;
    if ((refill_physical & PAGE_MASK) != 0) {
        return PAGING_INVALID_ARGUMENT;
    }

    paging_install_refill(refill_physical);
    return PAGING_OK;
}

static bool
is_canonical(uint64_t address) {
    uint64_t upper = address >> 48;
    return ((address >> 47) & 1) != 0 ? upper == UINT64_C(0xffff) : upper == 0;
}

static uint64_t *
table_from_physical(uint64_t physical) {
    return (uint64_t *)(uintptr_t)(ARCHWAY_HHDM_BASE + physical);
}

static enum paging_result
allocate_table(bool leaf, uint64_t *physical, uint64_t **table) {
    *physical = alloc_page();
    if (*physical == 0) {
        return PAGING_NO_MEMORY;
    }
    *table = table_from_physical(*physical);
    for (size_t i = 0; i < 512; ++i) {
        (*table)[i] = leaf ? PTE_GLOBAL : 0;
    }
    return PAGING_OK;
}

static uint64_t
page_table_entry_flags(enum paging_flags flags) {
    uint64_t entry_flags = PTE_VALID | PTE_GLOBAL;
    if ((flags & PAGING_WRITE) != 0) {
        entry_flags |= PTE_DIRTY;
    }
    if ((flags & PAGING_READ) == 0) {
        entry_flags |= PTE_NO_READ;
    }
    if ((flags & PAGING_EXECUTE) == 0) {
        entry_flags |= PTE_NO_EXEC;
    }
    if ((flags & PAGING_UNCACHED) == 0) {
        entry_flags |= PTE_MAT_COHERENT;
    }
    return entry_flags;
}

static enum paging_result
validate_page(uint64_t virtual_address, uint64_t physical_address) {
    if ((virtual_address & PAGE_MASK) != 0 || (physical_address & PAGE_MASK) != 0
        || !is_canonical(virtual_address) || physical_address > PHYSICAL_ADDRESS_MASK) {
        return PAGING_INVALID_ARGUMENT;
    }
    return PAGING_OK;
}

static enum paging_result
map_page_locked(uint64_t virtual_address, uint64_t physical_address, enum paging_flags flags) {
    enum paging_result result = validate_page(virtual_address, physical_address);
    if (result != PAGING_OK) {
        return result;
    }
    if (((uint32_t)flags
         & ~(uint32_t)(PAGING_READ | PAGING_WRITE | PAGING_EXECUTE | PAGING_UNCACHED))
        != 0) {
        return PAGING_INVALID_ARGUMENT;
    }

    uint64_t root_physical = paging_root_physical(virtual_address);
    bool new_low_root = false;
    if (root_physical == 0 && ((virtual_address >> 47) & 1) == 0) {
        root_physical = alloc_page();
        if (root_physical == 0) {
            return PAGING_NO_MEMORY;
        }
        if (root_physical > PHYSICAL_ADDRESS_MASK || (root_physical & PAGE_MASK) != 0) {
            dealloc_page(root_physical);
            return PAGING_INVALID_ARGUMENT;
        }
        uint64_t *new_root = table_from_physical(root_physical);
        for (size_t i = 0; i < 512; ++i) {
            new_root[i] = 0;
        }
        new_low_root = true;
    }
    if (root_physical == 0 || root_physical > PHYSICAL_ADDRESS_MAX
        || (root_physical & PAGE_MASK) != 0) {
        return PAGING_INVALID_ARGUMENT;
    }

    uint64_t *table = table_from_physical(root_physical);
    for (unsigned level = 3; level != 0; --level) {
        unsigned shift = PAGE_SHIFT + level * 9;
        size_t index = (size_t)((virtual_address >> shift) & PT_INDEX_MASK);
        uint64_t entry = table[index];
        if (entry != 0) {
            /* directory bit 6 is HUGE here 
               do not treat its PFN as a child table. */
            if ((entry & PDE_HUGE) != 0) {
                return PAGING_UNSUPPORTED;
            }
            table = table_from_physical(entry & PHYSICAL_ADDRESS_MASK);
            continue;
        }

        /* build the missing subtree privately */
        uint64_t allocated[3];
        size_t allocated_count = 0;
        uint64_t private_root = 0;
        uint64_t *private_parent = NULL;
        uint64_t *leaf_table = NULL;
        for (unsigned parent_level = level; parent_level != 0; --parent_level) {
            unsigned child_level = parent_level - 1;
            uint64_t child_physical;
            uint64_t *child_table;
            result = allocate_table(child_level == 0, &child_physical, &child_table);
            if (result != PAGING_OK) {
                for (size_t i = 0; i < allocated_count; ++i) {
                    dealloc_page(allocated[i]);
                }
                if (new_low_root) {
                    dealloc_page(root_physical);
                }
                return result;
            }
            allocated[allocated_count++] = child_physical;
            if (private_parent == NULL) {
                private_root = child_physical;
            } else {
                size_t child_index =
                    (size_t)((virtual_address >> (PAGE_SHIFT + parent_level * 9)) & PT_INDEX_MASK);
                private_parent[child_index] = child_physical | PDE_OWNED;
            }
            private_parent = child_table;
            if (child_level == 0) {
                leaf_table = child_table;
            }
        }

        size_t leaf_index = (size_t)((virtual_address >> PAGE_SHIFT) & PT_INDEX_MASK);
        leaf_table[leaf_index] = physical_address | page_table_entry_flags(flags);
        table[index] = private_root | PDE_OWNED;
        if (new_low_root) {
            paging_install_low_root(root_physical);
            low_root_owned = true;
        }
        paging_flush_tlb();
        return PAGING_OK;
    }

    size_t index = (size_t)((virtual_address >> PAGE_SHIFT) & PT_INDEX_MASK);
    uint64_t *pte = &table[index];
    if ((*pte & PTE_VALID) != 0 && (*pte & PHYSICAL_ADDRESS_MASK) != physical_address) {
        return PAGING_CONFLICT;
    }
    *pte = physical_address | page_table_entry_flags(flags);
    paging_flush_tlb();
    return PAGING_OK;
}

static enum paging_result
unmap_page_locked(uint64_t virtual_address, uint64_t *physical_address) {
    if ((virtual_address & PAGE_MASK) != 0 || !is_canonical(virtual_address)) {
        return PAGING_INVALID_ARGUMENT;
    }
    uint64_t root_physical = paging_root_physical(virtual_address);
    if (root_physical == 0) {
        return PAGING_CONFLICT;
    }
    if (root_physical > PHYSICAL_ADDRESS_MAX || (root_physical & PAGE_MASK) != 0) {
        return PAGING_INVALID_ARGUMENT;
    }

    uint64_t *parent_tables[3];
    size_t parent_indexes[3];
    uint64_t directory_entries[3];
    uint64_t *table = table_from_physical(root_physical);
    size_t depth = 0;
    for (unsigned level = 3; level != 0; --level) {
        unsigned shift = PAGE_SHIFT + level * 9;
        size_t index = (size_t)((virtual_address >> shift) & PT_INDEX_MASK);
        uint64_t entry = table[index];
        if (entry == 0) {
            return PAGING_CONFLICT;
        }
        if ((entry & PDE_HUGE) != 0) {
            return PAGING_UNSUPPORTED;
        }
        parent_tables[depth] = table;
        parent_indexes[depth] = index;
        directory_entries[depth] = entry;
        ++depth;
        table = table_from_physical(entry & PHYSICAL_ADDRESS_MASK);
    }

    size_t index = (size_t)((virtual_address >> PAGE_SHIFT) & PT_INDEX_MASK);
    uint64_t entry = table[index];
    if ((entry & PTE_VALID) == 0) {
        return PAGING_CONFLICT;
    }
    table[index] = PTE_GLOBAL;
    paging_flush_tlb();

    /* reclaim tables allocated by kernel */
    uint64_t to_free[3];
    size_t free_count = 0;
    uint64_t *candidate = table;
    for (size_t i = depth; i != 0; --i) {
        size_t path_index = i - 1;
        uint64_t directory_entry = directory_entries[path_index];
        if ((directory_entry & PDE_OWNED) == 0) {
            break;
        }

        bool empty = true;
        for (size_t entry_index = 0; entry_index < 512; ++entry_index) {
            if (path_index == depth - 1) {
                if ((candidate[entry_index] & PTE_VALID) != 0) {
                    empty = false;
                    break;
                }
            } else if (candidate[entry_index] != 0) {
                empty = false;
                break;
            }
        }
        if (!empty) {
            break;
        }

        to_free[free_count++] = directory_entry & PHYSICAL_ADDRESS_MASK;
        parent_tables[path_index][parent_indexes[path_index]] = 0;
        candidate = parent_tables[path_index];
    }
    if (free_count != 0) {
        /* todo: add remote shootdowns when we get smp */
        paging_flush_tlb();
        for (size_t i = 0; i < free_count; ++i) {
            dealloc_page(to_free[i]);
        }
    }

    if (((virtual_address >> 47) & 1) == 0 && low_root_owned) {
        bool empty = true;
        for (size_t entry_index = 0; entry_index < 512; ++entry_index) {
            if (table_from_physical(root_physical)[entry_index] != 0) {
                empty = false;
                break;
            }
        }
        if (empty) {
            paging_install_low_root(0);
            low_root_owned = false;
            dealloc_page(root_physical);
        }
    }
    if (physical_address != NULL) {
        *physical_address = entry & PHYSICAL_ADDRESS_MASK;
    }
    return PAGING_OK;
}

enum paging_result
paging_map_page(uint64_t virtual_address, uint64_t physical_address, enum paging_flags flags) {
    spin_lock(&page_table_lock);
    enum paging_result result = map_page_locked(virtual_address, physical_address, flags);
    spin_unlock(&page_table_lock);
    return result;
}

enum paging_result
paging_unmap_page(uint64_t virtual_address, uint64_t *physical_address) {
    spin_lock(&page_table_lock);
    enum paging_result result = unmap_page_locked(virtual_address, physical_address);
    spin_unlock(&page_table_lock);
    return result;
}
