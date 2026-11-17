/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file main.c
 * @brief Archway entry point
 */

#include <drivers/serial/serial.h>
#include <mem/mm.h>
#include <log.h>
#include <archway/boot_info.h>

_Noreturn void
kernel_main(const struct archway_boot_info *boot_info) {
    serial_init();
    kprintf("Hello, world!\n");
    mm_print_memory_map(boot_info);

    for (;;) {
        __asm__ volatile("nop");
    }
}
