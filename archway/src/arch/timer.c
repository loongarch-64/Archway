/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file timer.c
 * @brief LoongArch CPU-local timer.
 */

#include <arch/loongarch.h>
#include <arch/timer.h>
#include <log.h>
#include <stdatomic.h>

#define CPUCFG_FEATURES 2
#define CPUCFG_LLFTP (UINT32_C(1) << 14)
#define CPUCFG_CLOCK_BASE_HZ 4
#define CPUCFG_CLOCK_RATIO 5
#define CPUCFG_CLOCK_MULTIPLIER_MASK UINT32_C(0xffff)
#define CPUCFG_CLOCK_DIVISOR_SHIFT 16
#define PRCFG1_TIMER_BITS_SHIFT 4
#define PRCFG1_TIMER_BITS_MASK 0xffu
#define TCFG_ENABLE UINT64_C(1)
#define TCFG_PERIODIC UINT64_C(2)
#define TCFG_CONTROL_MASK (TCFG_ENABLE | TCFG_PERIODIC)
#define TICLR_CLEAR UINT64_C(1)

static _Atomic uint64_t interrupt_count;

static uint32_t
cpu_config(unsigned word) {
    uint64_t value;
    __asm__ volatile("cpucfg %0, %1" : "=r"(value) : "r"((uint64_t)word));
    return (uint32_t)value;
}

static void
timer_acknowledge(void) {
    uint64_t clear = TICLR_CLEAR;
    __asm__ volatile("csrwr %0, %1" : "+r"(clear) : "i"(LOONGARCH_CSR_TICLR) : "memory");
}

static void
timer_set_irq_enabled(bool enabled) {
    uint64_t value = enabled ? LOONGARCH_INT_TIMER : 0;
    __asm__ volatile("csrxchg %0, %1, %2"
                     : "+&r"(value)
                     : "r"((uint64_t)LOONGARCH_INT_TIMER), "i"(LOONGARCH_CSR_ECFG)
                     : "memory");
}

bool
timer_start_periodic(uint64_t ticks) {
    uint32_t features = cpu_config(CPUCFG_FEATURES);
    if ((features & CPUCFG_LLFTP) == 0 || ticks == 0 || (ticks & TCFG_CONTROL_MASK) != 0) {
        return false;
    }

    uint64_t prcfg1;
    __asm__ volatile("csrrd %0, %1" : "=r"(prcfg1) : "i"(LOONGARCH_CSR_PRCFG1));
    unsigned last_bit = (prcfg1 >> PRCFG1_TIMER_BITS_SHIFT) & PRCFG1_TIMER_BITS_MASK;
    if (last_bit < 63 && (ticks >> (last_bit + 1)) != 0) {
        return false;
    }

    timer_set_irq_enabled(false);
    __asm__ volatile("csrwr $zero, %0" : : "i"(LOONGARCH_CSR_TCFG) : "memory");
    timer_acknowledge();
    uint64_t config = ticks | TCFG_ENABLE | TCFG_PERIODIC;
    __asm__ volatile("csrwr %0, %1" : "+r"(config) : "i"(LOONGARCH_CSR_TCFG) : "memory");
    timer_set_irq_enabled(true);
    return true;
}

void
timer_handle_interrupt(void) {
    timer_acknowledge();
    atomic_fetch_add_explicit(&interrupt_count, 1, memory_order_relaxed);
    kprintf(".");
}

uint64_t
timer_interrupt_count(void) {
    return atomic_load_explicit(&interrupt_count, memory_order_relaxed);
}

uint64_t
timer_now_us(void) {
    if ((cpu_config(CPUCFG_FEATURES) & CPUCFG_LLFTP) == 0) {
        return 0;
    }

    uint64_t base_hz = cpu_config(CPUCFG_CLOCK_BASE_HZ);
    uint32_t ratio = cpu_config(CPUCFG_CLOCK_RATIO);
    uint32_t multiplier = ratio & CPUCFG_CLOCK_MULTIPLIER_MASK;
    uint32_t divisor = ratio >> CPUCFG_CLOCK_DIVISOR_SHIFT;
    if (divisor == 0) {
        return 0;
    }
    uint64_t frequency = base_hz * multiplier / divisor;
    if (frequency == 0) {
        return 0;
    }

    uint64_t ticks;
    __asm__ volatile("rdtime.d %0, $zero" : "=r"(ticks) : : "memory");

    uint64_t seconds = ticks / frequency;
    uint64_t remainder = (ticks % frequency) * 1000;
    uint64_t milliseconds = remainder / frequency;
    uint64_t microseconds = ((remainder % frequency) * 1000) / frequency;
    return seconds * 1000000 + milliseconds * 1000 + microseconds;
}
