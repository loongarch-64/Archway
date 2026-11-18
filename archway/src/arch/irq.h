/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file irq.h
 * @brief CPU-local interrupt state.
 */

#ifndef ARCHWAY_ARCH_IRQ_H
#define ARCHWAY_ARCH_IRQ_H

#include <arch/loongarch.h>
#include <stdint.h>

static inline uint64_t
interrupt_save_disable(void) {
    uint64_t state = 0;
    __asm__ volatile("csrxchg %0, %1, %2"
                     : "+&r"(state)
                     : "r"((uint64_t)LOONGARCH_CRMD_IE), "i"(LOONGARCH_CSR_CRMD)
                     : "memory");
    return state & LOONGARCH_CRMD_IE;
}

static inline void
interrupt_restore(uint64_t state) {
    __asm__ volatile("csrxchg %0, %1, %2"
                     : "+&r"(state)
                     : "r"((uint64_t)LOONGARCH_CRMD_IE), "i"(LOONGARCH_CSR_CRMD)
                     : "memory");
}

#endif
