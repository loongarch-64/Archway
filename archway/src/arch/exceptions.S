/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file exceptions.S
 * @brief EENTRY initialization.
 */

#include "loongarch.h"

.section .text, "ax", @progbits
.balign 4
.global exception_init
.type exception_init, @function
exception_init:
    la.local $t0, exception_entry
    csrwr   $t0, LOONGARCH_CSR_EENTRY
    jirl    $zero, $ra, 0
.size exception_init, . - exception_init

.global interrupt_enable
.type interrupt_enable, @function
interrupt_enable:
    li.w    $t0, LOONGARCH_CRMD_IE
    move    $t1, $t0
    csrxchg $t1, $t0, LOONGARCH_CSR_CRMD
    jr      $ra
.size interrupt_enable, . - interrupt_enable
