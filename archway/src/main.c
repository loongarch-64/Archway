/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file main.c
 * @brief Archway entry point
 */

#include <arch/hcf.h>
#include <arch/interrupt_frame.h>
#include <arch/paging.h>
#include <archway/boot_info.h>
#include <drivers/serial/serial.h>
#include <log.h>
#include <mem/mm.h>

_Noreturn void
kernel_main(const struct archway_boot_info *boot_info) {
    exception_init();
    serial_init();
    /* install kernel vectors before initializing allocation 
       or reclaiming bootloader memory */
    if (paging_init(boot_info->kernel_physical_base) != PAGING_OK) {
        kprintf("paging: unable to load kernel TLB refill handler\n");
        hcf();
    }
    kprintf("paging: loaded a new tlb refill handler\n");
    mm_print_memory_map(boot_info);
    mm_init(boot_info);
    kprintf("Hello, world!\n");

    uint64_t page = alloc_page();
    uint64_t page2 = alloc_page();

    kprintf("page = %llX, page2 = %llX\n", page, page2);

    __asm__ volatile("break 0");
    hcf();
}
