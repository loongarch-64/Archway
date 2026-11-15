/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file entry.S
 * @brief Kernel's entry point.
 */

.section .text.entry, "ax", @progbits
.balign 4
.global _start
.type _start, @function
_start:
    /* The bootloader supplies an aligned stack in the HHDM. */
    bl kernel_main
1:
    b 1b
.size _start, . - _start
