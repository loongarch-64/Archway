/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file heap.c
 * @brief Size class slab allocator.
 */

#include <arch/hcf.h>
#include <log.h>
#include <mem/heap.h>
#include <mem/mm.h>
#include <stdbool.h>
#include <stdint.h>
#include <sync/spinlock.h>
#include <vm/map.h>
#include <vm/reserve.h>

#define SLAB_REGION_PAGES 64
#define SLAB_REGION_SIZE (SLAB_REGION_PAGES * MM_PAGE_SIZE)
#define SLAB_REGION_FREE_PAGES                                                                     \
    (UINT64_MAX & ~UINT64_C(1)) /* the first page stores the region metadata */

struct free_slot {
    struct free_slot *next;
};

struct slab_class {
    size_t slot_size;
    struct slab *partial;
};

struct slab_region {
    struct slab_region *previous;
    struct slab_region *next;
    uint64_t free_pages;
};

struct heap_page {
    /* zero means a slab page. */
    _Alignas(max_align_t) size_t mapped_size;
};

struct slab {
    struct heap_page page;
    struct slab_region *region;
    struct slab_class *class;
    struct free_slot *free_slots;
    struct slab *previous;
    struct slab *next;
    size_t used;
};

/* slab operations may take VM locks and the physical allocator lock */
static spinlock_t slab_lock = SPINLOCK_INITIALIZER;
static struct slab_region *available_regions;
static struct slab_class slab_classes[] = {
    {.slot_size = 16},
    {.slot_size = 32},
    {.slot_size = 64},
    {.slot_size = 128},
    {.slot_size = 256},
    {.slot_size = 512},
    {.slot_size = 1024},
};

_Static_assert(16 % _Alignof(max_align_t) == 0, "slab slots must be maximally aligned");

static bool
map_heap_page(uintptr_t address) {
    uint64_t physical = alloc_page();
    if (physical == 0) {
        return false;
    }
    if (vm_map_range(address, physical, MM_PAGE_SIZE, VM_MAP_READ | VM_MAP_WRITE) != VM_MAP_OK) {
        dealloc_page(physical);
        return false;
    }
    return true;
}

static void
unmap_heap_page(uintptr_t address) {
    uint64_t physical;
    if (vm_unmap_page(address, &physical) != VM_MAP_OK) {
        kprintf("heap: unable to unmap allocation page\n");
        hcf();
    }
    dealloc_page(physical);
}

static void
release_heap_range(uintptr_t base, size_t size) {
    if (!vm_release((void *)base, size)) {
        kprintf("heap: unable to release allocation range\n");
        hcf();
    }
}

static void
link_available_region(struct slab_region *region) {
    region->previous = NULL;
    region->next = available_regions;
    if (region->next != NULL) {
        region->next->previous = region;
    }
    available_regions = region;
}

static void
unlink_available_region(struct slab_region *region) {
    if (region->previous != NULL) {
        region->previous->next = region->next;
    } else {
        available_regions = region->next;
    }
    if (region->next != NULL) {
        region->next->previous = region->previous;
    }
}

static struct slab_region *
allocate_slab_region(void) {
    struct slab_region *region = vm_reserve(VM_REGION_VMALLOC, SLAB_REGION_SIZE);
    if (region == NULL) {
        return NULL;
    }
    if (!map_heap_page((uintptr_t)region)) {
        release_heap_range((uintptr_t)region, SLAB_REGION_SIZE);
        return NULL;
    }
    region->free_pages = SLAB_REGION_FREE_PAGES;
    link_available_region(region);
    return region;
}

static void
release_slab_region(struct slab_region *region) {
    unlink_available_region(region);
    uintptr_t base = (uintptr_t)region;
    unmap_heap_page(base);
    release_heap_range(base, SLAB_REGION_SIZE);
}

static void
link_partial(struct slab *slab) {
    slab->previous = NULL;
    slab->next = slab->class->partial;
    if (slab->next != NULL) {
        slab->next->previous = slab;
    }
    slab->class->partial = slab;
}

static void
unlink_partial(struct slab *slab) {
    if (slab->previous != NULL) {
        slab->previous->next = slab->next;
    } else {
        slab->class->partial = slab->next;
    }
    if (slab->next != NULL) {
        slab->next->previous = slab->previous;
    }
}

static struct slab *
allocate_slab(struct slab_class *class) {
    struct slab_region *region = available_regions;
    if (region == NULL) {
        region = allocate_slab_region();
        if (region == NULL) {
            return NULL;
        }
    }

    unsigned index = (unsigned)__builtin_ctzll(region->free_pages);
    uintptr_t address = (uintptr_t)region + index * MM_PAGE_SIZE;
    if (!map_heap_page(address)) {
        if (region->free_pages == SLAB_REGION_FREE_PAGES) {
            release_slab_region(region);
        }
        return NULL;
    }
    region->free_pages &= ~(UINT64_C(1) << index);
    if (region->free_pages == 0) {
        unlink_available_region(region);
    }

    struct slab *slab = (void *)address;
    slab->page.mapped_size = 0;
    slab->region = region;
    slab->class = class;
    slab->free_slots = NULL;
    slab->used = 0;
    for (size_t offset = sizeof(*slab); offset <= MM_PAGE_SIZE - class->slot_size;
         offset += class->slot_size) {
        struct free_slot *slot = (void *)((unsigned char *)slab + offset);
        slot->next = slab->free_slots;
        slab->free_slots = slot;
    }
    link_partial(slab);
    return slab;
}

static void *
allocate_slot(struct slab_class *class) {
    struct slab *slab = class->partial;
    if (slab == NULL) {
        slab = allocate_slab(class);
        if (slab == NULL) {
            return NULL;
        }
    }

    struct free_slot *slot = slab->free_slots;
    slab->free_slots = slot->next;
    ++slab->used;
    if (slab->free_slots == NULL) {
        unlink_partial(slab);
    }
    return slot;
}

static void
release_slot(uintptr_t base, struct free_slot *slot) {
    struct slab *slab = (void *)base;
    if (slab->free_slots == NULL) {
        link_partial(slab);
    }
    slot->next = slab->free_slots;
    slab->free_slots = slot;
    if (--slab->used == 0) {
        unlink_partial(slab);
        struct slab_region *region = slab->region;
        size_t index = (base - (uintptr_t)region) / MM_PAGE_SIZE;
        unmap_heap_page(base);
        if (region->free_pages == 0) {
            link_available_region(region);
        }
        region->free_pages |= UINT64_C(1) << index;
        if (region->free_pages == SLAB_REGION_FREE_PAGES) {
            release_slab_region(region);
        }
    }
}

static void
release_large(uintptr_t base, size_t reserved_size, size_t mapped_size) {
    while (mapped_size != 0) {
        mapped_size -= MM_PAGE_SIZE;
        unmap_heap_page(base + mapped_size);
    }
    release_heap_range(base, reserved_size);
}

static void *
allocate_large(size_t size) {
    if (size > SIZE_MAX - sizeof(struct heap_page) - (MM_PAGE_SIZE - 1)) {
        return NULL;
    }
    size_t mapped_size = (size + sizeof(struct heap_page) + MM_PAGE_SIZE - 1) & ~(MM_PAGE_SIZE - 1);
    struct heap_page *allocation = vm_reserve(VM_REGION_VMALLOC, mapped_size);
    if (allocation == NULL) {
        return NULL;
    }

    uintptr_t base = (uintptr_t)allocation;
    for (size_t offset = 0; offset < mapped_size; offset += MM_PAGE_SIZE) {
        if (!map_heap_page(base + offset)) {
            release_large(base, mapped_size, offset);
            return NULL;
        }
    }
    allocation->mapped_size = mapped_size;
    return allocation + 1;
}

void *
kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    for (size_t i = 0; i < sizeof(slab_classes) / sizeof(slab_classes[0]); ++i) {
        if (size <= slab_classes[i].slot_size) {
            spin_lock(&slab_lock);
            void *slot = allocate_slot(&slab_classes[i]);
            spin_unlock(&slab_lock);
            return slot;
        }
    }
    return allocate_large(size);
}

void
kfree(void *pointer) {
    if (pointer == NULL) {
        return;
    }
    uintptr_t base = (uintptr_t)pointer & ~(MM_PAGE_SIZE - 1);
    const struct heap_page *page = (void *)base;
    size_t mapped_size = page->mapped_size;
    if (mapped_size == 0) {
        spin_lock(&slab_lock);
        release_slot(base, pointer);
        spin_unlock(&slab_lock);
    } else {
        release_large(base, mapped_size, mapped_size);
    }
}
