/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file main.c
 * @brief Archway entry point
 */

#include "serial.h"

_Noreturn void
kernel_main(void) {
    serial_init();
    serial_write("Hello, world!\n");

    for (;;) {
        __asm__ volatile("nop");
    }
}
