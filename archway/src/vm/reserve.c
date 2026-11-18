/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file reserve.c
 * @brief First-fit allocation of kernel virtual address ranges.
 */

#include <archway/memory_layout.h>
#include <mem/mm.h>
#include <sync/spinlock.h>
#include <vm/reserve.h>

/* AVL nodes track the preceding gap 
   and the largest such gap in their subtree */
struct vm_reservation {
    struct vm_reservation *left;
    struct vm_reservation *right;
    uint64_t offset;
    uint64_t size;
    uint64_t gap_before;
    uint64_t max_gap;
    int height;
};

struct reservation_page {
    struct reservation_page *previous;
    struct reservation_page *next;
    struct vm_reservation *free_records;
    size_t used;
    struct vm_reservation records[];
};

#define RECORDS_PER_PAGE                                                                           \
    ((MM_PAGE_SIZE - offsetof(struct reservation_page, records)) / sizeof(struct vm_reservation))

struct vm_arena {
    uint64_t base;
    uint64_t size;
    uint64_t tail; /* end of the last reservation, 
                      relative to base */
    struct vm_reservation *root;
};

/* offsets are used so the drivers region
   can end one byte past UINT64_MAX */
static struct vm_arena arenas[] = {
    [VM_REGION_VMALLOC] =
        {
            .base = ARCHWAY_VMALLOC_BASE,
            .size = ARCHWAY_VMALLOC_END - ARCHWAY_VMALLOC_BASE,
        },
    [VM_REGION_DRIVERS] = {
        .base = ARCHWAY_DRIVERS_BASE,
        .size = ARCHWAY_DRIVERS_SIZE,
    },
};

/* only pages with room for another reservation are linked here */
static struct reservation_page *available_pages;
/* covers the arenas and their shared reservation-record pool */
static spinlock_t reservation_lock = SPINLOCK_INITIALIZER;

static void
link_available_page(struct reservation_page *page) {
    page->previous = NULL;
    page->next = available_pages;
    if (page->next != NULL) {
        page->next->previous = page;
    }
    available_pages = page;
}

static void
unlink_available_page(struct reservation_page *page) {
    if (page->previous != NULL) {
        page->previous->next = page->next;
    } else {
        available_pages = page->next;
    }
    if (page->next != NULL) {
        page->next->previous = page->previous;
    }
}

static struct vm_reservation *
alloc_record(void) {
    struct reservation_page *page = available_pages;
    if (page == NULL) {
        uint64_t physical = alloc_page();
        if (physical == 0) {
            return NULL;
        }

        /* the direct map lets the pool grow without 
           reserving virtual memory */
        page = (void *)(uintptr_t)(ARCHWAY_HHDM_BASE + physical);
        page->free_records = NULL;
        page->used = 0;
        link_available_page(page);
    }

    struct vm_reservation *record = page->free_records;
    if (record != NULL) {
        page->free_records = record->left;
    } else {
        /* with no recycled records, every initialized slot is live */
        record = &page->records[page->used];
    }
    if (++page->used == RECORDS_PER_PAGE) {
        unlink_available_page(page);
    }
    return record;
}

static void
release_record(struct vm_reservation *record) {
    uintptr_t address = (uintptr_t)record & ~(MM_PAGE_SIZE - 1);
    struct reservation_page *page = (void *)address;
    if (page->used == RECORDS_PER_PAGE) {
        link_available_page(page);
    }
    if (--page->used == 0) {
        unlink_available_page(page);
        dealloc_page(address - ARCHWAY_HHDM_BASE);
    } else {
        record->left = page->free_records;
        page->free_records = record;
    }
}

static int
height(const struct vm_reservation *node) {
    return node == NULL ? 0 : node->height;
}

static uint64_t
max_gap(const struct vm_reservation *node) {
    return node == NULL ? 0 : node->max_gap;
}

static void
refresh(struct vm_reservation *node) {
    int left_height = height(node->left);
    int right_height = height(node->right);
    node->height = 1 + (left_height > right_height ? left_height : right_height);

    uint64_t left_gap = max_gap(node->left);
    uint64_t right_gap = max_gap(node->right);
    uint64_t child_gap = left_gap > right_gap ? left_gap : right_gap;
    node->max_gap = node->gap_before > child_gap ? node->gap_before : child_gap;
}

static struct vm_reservation *
rotate_left(struct vm_reservation *node) {
    struct vm_reservation *top = node->right;
    node->right = top->left;
    top->left = node;
    refresh(node);
    refresh(top);
    return top;
}

static struct vm_reservation *
rotate_right(struct vm_reservation *node) {
    struct vm_reservation *top = node->left;
    node->left = top->right;
    top->right = node;
    refresh(node);
    refresh(top);
    return top;
}

static struct vm_reservation *
rebalance(struct vm_reservation *node) {
    int balance = height(node->left) - height(node->right);
    if (balance > 1) {
        if (height(node->left->left) < height(node->left->right)) {
            node->left = rotate_left(node->left);
        }
        return rotate_right(node);
    }
    if (balance < -1) {
        if (height(node->right->right) < height(node->right->left)) {
            node->right = rotate_right(node->right);
        }
        return rotate_left(node);
    }
    refresh(node);
    return node;
}

static struct vm_reservation *
insert(struct vm_reservation *node, struct vm_reservation *record) {
    if (node == NULL) {
        return record;
    }
    if (record->offset < node->offset) {
        node->left = insert(node->left, record);
    } else {
        node->right = insert(node->right, record);
    }
    return rebalance(node);
}

static struct vm_reservation *
first_reservation(struct vm_reservation *node) {
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}

static struct vm_reservation *
remove_reservation(struct vm_reservation *node, uint64_t offset) {
    if (offset < node->offset) {
        node->left = remove_reservation(node->left, offset);
    } else if (offset > node->offset) {
        node->right = remove_reservation(node->right, offset);
    } else {
        if (node->left == NULL) {
            return node->right;
        }
        if (node->right == NULL) {
            return node->left;
        }

        struct vm_reservation *successor = first_reservation(node->right);
        successor->right = remove_reservation(node->right, successor->offset);
        successor->left = node->left;
        node = successor;
    }
    return rebalance(node);
}

static void
set_gap(struct vm_reservation *node, uint64_t offset, uint64_t gap) {
    if (offset < node->offset) {
        set_gap(node->left, offset, gap);
    } else if (offset > node->offset) {
        set_gap(node->right, offset, gap);
    } else {
        node->gap_before = gap;
    }
    refresh(node);
}

static struct vm_reservation *
find_first_fit(struct vm_reservation *node, uint64_t size) {
    while (node != NULL) {
        if (max_gap(node->left) >= size) {
            node = node->left;
        } else if (node->gap_before >= size) {
            return node;
        } else {
            node = node->right;
        }
    }
    return NULL;
}

static uint64_t
round_reservation_size(size_t length) {
    if (length > SIZE_MAX - (MM_PAGE_SIZE - 1)) {
        return 0;
    }
    return (length + MM_PAGE_SIZE - 1) & ~(MM_PAGE_SIZE - 1);
}

static void *
reserve_from_arena(struct vm_arena *arena, uint64_t size) {
    struct vm_reservation *next = find_first_fit(arena->root, size);
    uint64_t offset = next == NULL ? arena->tail : next->offset - next->gap_before;
    if (arena->size - offset < size) {
        return NULL;
    }

    struct vm_reservation *record = alloc_record();
    if (record == NULL) {
        return NULL;
    }

    /* first fit uses the start of the gap, 
       leaving any remainder before next */
    if (next != NULL) {
        set_gap(arena->root, next->offset, next->gap_before - size);
    } else {
        arena->tail = offset + size;
    }

    *record = (struct vm_reservation){.offset = offset, .size = size, .height = 1};
    arena->root = insert(arena->root, record);
    return (void *)(uintptr_t)(arena->base + offset);
}

void *
vm_reserve(enum vm_region region, size_t length) {
    uint64_t size = round_reservation_size(length);
    if (size == 0 || (unsigned)region >= sizeof(arenas) / sizeof(arenas[0])) {
        return NULL;
    }

    spin_lock(&reservation_lock);
    void *base = reserve_from_arena(&arenas[region], size);
    spin_unlock(&reservation_lock);
    return base;
}

static bool
release_from_arena(struct vm_arena *arena, uint64_t offset, uint64_t size) {
    struct vm_reservation *next = NULL;
    struct vm_reservation *record = arena->root;
    while (record != NULL && record->offset != offset) {
        if (offset < record->offset) {
            next = record;
            record = record->left;
        } else {
            record = record->right;
        }
    }
    if (record == NULL || record->size != size) {
        return false;
    }

    if (record->right != NULL) {
        next = first_reservation(record->right);
    }
    if (next != NULL) {
        set_gap(arena->root, next->offset, next->gap_before + record->size + record->gap_before);
    } else {
        arena->tail = record->offset - record->gap_before;
    }

    arena->root = remove_reservation(arena->root, offset);
    release_record(record);
    return true;
}

bool
vm_release(void *base, size_t length) {
    uint64_t size = round_reservation_size(length);
    if (size == 0) {
        return false;
    }

    uint64_t address = (uintptr_t)base;
    for (size_t i = 0; i < sizeof(arenas) / sizeof(arenas[0]); ++i) {
        uint64_t offset = address - arenas[i].base;
        if (offset < arenas[i].size) {
            spin_lock(&reservation_lock);
            bool released = release_from_arena(&arenas[i], offset, size);
            spin_unlock(&reservation_lock);
            return released;
        }
    }
    return false;
}
