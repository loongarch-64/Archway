/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file main.c
 * @brief Archway entry point
 */

#include <arch/hcf.h>
#include <arch/interrupt_frame.h>
#include <arch/paging.h>
#include <arch/timer.h>
#include <archway/boot_info.h>
#include <drivers/serial/serial.h>
#include <log.h>
#include <mem/heap.h>
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

    if (!timer_start_periodic(1000000)) {
        kprintf("timer: unable to start periodic timer\n");
        hcf();
    }

    kprintf("timer: periodic mode enabled\n");
    interrupt_enable();
    while (timer_interrupt_count() == 0) {
        __asm__ volatile("idle 0" ::: "memory");
    }

    uint64_t time_now_us = timer_now_us();
    kprintf(
        "time since cpu reset: %llu.%06u s\n",
        (unsigned long long)(time_now_us / 1000000),
        (unsigned int)(time_now_us % 1000000)
    );

    void *p1 = kmalloc(623);
    void *p2 = kmalloc(76);

    kprintf("p1=%p p2=%p\n", p1, p2);

    hcf();
}
