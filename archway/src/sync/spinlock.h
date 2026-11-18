/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file spinlock.h
 * @brief Non-recursive kernel spinlocks.
 */

#ifndef ARCHWAY_SYNC_SPINLOCK_H
#define ARCHWAY_SYNC_SPINLOCK_H

#include <arch/irq.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef struct {
    atomic_uint locked;
} spinlock_t;

#define SPINLOCK_INITIALIZER {.locked = 0}

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "spinlocks require lock-free atomics");

static inline void
spinlock_init(spinlock_t *lock) {
    atomic_init(&lock->locked, 0);
}

static inline bool
spin_trylock(spinlock_t *lock) {
    unsigned expected = 0;
    return atomic_compare_exchange_strong_explicit(
        &lock->locked, &expected, 1, memory_order_acquire, memory_order_relaxed
    );
}

static inline void
spin_lock(spinlock_t *lock) {
    while (!spin_trylock(lock)) {
        while (atomic_load_explicit(&lock->locked, memory_order_relaxed) != 0) {
        }
    }
}

static inline void
spin_unlock(spinlock_t *lock) {
    atomic_store_explicit(&lock->locked, 0, memory_order_release);
}

static inline uint64_t
spin_lock_irqsave(spinlock_t *lock) {
    uint64_t state = interrupt_save_disable();
    spin_lock(lock);
    return state;
}

static inline void
spin_unlock_irqrestore(spinlock_t *lock, uint64_t state) {
    spin_unlock(lock);
    interrupt_restore(state);
}

#endif
