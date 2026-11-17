/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file entry.S
 * @brief take a guess.
 */

.section .text.entry, "ax", @progbits
.balign 4
.global _start
.type _start, @function
_start:
    la.local $sp, __kernel_stack_top
    move $fp, $zero
    bl kernel_main
1:
    b 1b
.size _start, . - _start

.section .bss.stack, "aw", @nobits
.balign 16
.global __kernel_stack_bottom
__kernel_stack_bottom:
    .zero 65536
.global __kernel_stack_top
__kernel_stack_top:
