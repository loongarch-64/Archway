/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file hcf.c
 * @brief Rayan
 */

#include <arch/hcf.h>

_Noreturn void
hcf(void) {
    for (;;) {
        __asm__ volatile("idle 0");
    }
}
