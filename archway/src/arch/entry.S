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
    bl kernel_main
1:
    b 1b
.size _start, . - _start
