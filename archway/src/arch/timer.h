/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file timer.h
 * @brief LoongArch CPU-local timer.
 */

#ifndef ARCHWAY_ARCH_TIMER_H
#define ARCHWAY_ARCH_TIMER_H

#include <stdbool.h>
#include <stdint.h>

/* start the local timer in periodic mode and unmask its IRQ.
   ticks must be a positive multiple of four. 
   
   returns false if the timer is unsupported
   or the interval is out of range. */
bool timer_start_periodic(uint64_t ticks);
void timer_handle_interrupt(void);
uint64_t timer_interrupt_count(void);

/* Read the monotonic hardware counter in microseconds. Returns zero if unavailable. */
uint64_t timer_now_us(void);

#endif /* ARCHWAY_ARCH_TIMER_H */
