/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file main.c
 * @brief Archway entry point
 */

#include <arch/hcf.h>
#include <archway/boot_info.h>
#include <drivers/serial/serial.h>
#include <log.h>
#include <mem/mm.h>

_Noreturn void
kernel_main(const struct archway_boot_info *boot_info) {
    serial_init();
    kprintf("Hello, world!\n");
    mm_print_memory_map(boot_info);
    mm_init(boot_info);

    uint64_t page = alloc_page();
    uint64_t page2 = alloc_page();

    kprintf("page = %llX, page2 = %llX\n", page, page2);

    hcf();
}
