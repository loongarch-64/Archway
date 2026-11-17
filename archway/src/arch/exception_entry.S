/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file exception_entry.S
 * @brief Exception vector stub.
 */

#include "loongarch.h"
#include "interrupt_frame.h"

.section .text, "ax", @progbits
.balign 64
.global exception_entry
.hidden exception_entry
.type exception_entry, @function
exception_entry:
    /* SAVE0 and SAVE1 belong to TLB refill
       normal entry uses SAVE3. */
    csrwr   $t0, LOONGARCH_CSR_SAVE3

    /* todo: fetch sp from somewhere else instead of using interrupted sp */
    addi.d  $sp, $sp, -INTERRUPT_FRAME_SIZE

    st.d    $zero, $sp, INTERRUPT_FRAME_GPR_OFFSET(0)
    st.d    $ra,   $sp, INTERRUPT_FRAME_GPR_OFFSET(1)
    st.d    $tp,   $sp, INTERRUPT_FRAME_GPR_OFFSET(2)
    /* save interrupted sp + fixup */
    addi.d  $t0, $sp, INTERRUPT_FRAME_SIZE
    st.d    $t0,   $sp, INTERRUPT_FRAME_GPR_OFFSET(3)
    st.d    $a0,   $sp, INTERRUPT_FRAME_GPR_OFFSET(4)
    st.d    $a1,   $sp, INTERRUPT_FRAME_GPR_OFFSET(5)
    st.d    $a2,   $sp, INTERRUPT_FRAME_GPR_OFFSET(6)
    st.d    $a3,   $sp, INTERRUPT_FRAME_GPR_OFFSET(7)
    st.d    $a4,   $sp, INTERRUPT_FRAME_GPR_OFFSET(8)
    st.d    $a5,   $sp, INTERRUPT_FRAME_GPR_OFFSET(9)
    st.d    $a6,   $sp, INTERRUPT_FRAME_GPR_OFFSET(10)
    st.d    $a7,   $sp, INTERRUPT_FRAME_GPR_OFFSET(11)
    st.d    $t0,   $sp, INTERRUPT_FRAME_GPR_OFFSET(12) /* replaced by saved $t0 later on */
    st.d    $t1,   $sp, INTERRUPT_FRAME_GPR_OFFSET(13)
    st.d    $t2,   $sp, INTERRUPT_FRAME_GPR_OFFSET(14)
    st.d    $t3,   $sp, INTERRUPT_FRAME_GPR_OFFSET(15)
    st.d    $t4,   $sp, INTERRUPT_FRAME_GPR_OFFSET(16)
    st.d    $t5,   $sp, INTERRUPT_FRAME_GPR_OFFSET(17)
    st.d    $t6,   $sp, INTERRUPT_FRAME_GPR_OFFSET(18)
    st.d    $t7,   $sp, INTERRUPT_FRAME_GPR_OFFSET(19)
    st.d    $t8,   $sp, INTERRUPT_FRAME_GPR_OFFSET(20)
    st.d    $r21,  $sp, INTERRUPT_FRAME_GPR_OFFSET(21)
    st.d    $fp,   $sp, INTERRUPT_FRAME_GPR_OFFSET(22)
    st.d    $s0,   $sp, INTERRUPT_FRAME_GPR_OFFSET(23)
    st.d    $s1,   $sp, INTERRUPT_FRAME_GPR_OFFSET(24)
    st.d    $s2,   $sp, INTERRUPT_FRAME_GPR_OFFSET(25)
    st.d    $s3,   $sp, INTERRUPT_FRAME_GPR_OFFSET(26)
    st.d    $s4,   $sp, INTERRUPT_FRAME_GPR_OFFSET(27)
    st.d    $s5,   $sp, INTERRUPT_FRAME_GPR_OFFSET(28)
    st.d    $s6,   $sp, INTERRUPT_FRAME_GPR_OFFSET(29)
    st.d    $s7,   $sp, INTERRUPT_FRAME_GPR_OFFSET(30)
    st.d    $s8,   $sp, INTERRUPT_FRAME_GPR_OFFSET(31)
    csrrd   $t0, LOONGARCH_CSR_SAVE3 /* interrupted $t0 */
    st.d    $t0, $sp, INTERRUPT_FRAME_GPR_OFFSET(12)
    csrrd   $t0, LOONGARCH_CSR_ERA
    st.d    $t0, $sp, INTERRUPT_FRAME_ERA_OFFSET
    csrrd   $t0, LOONGARCH_CSR_ESTAT
    st.d    $t0, $sp, INTERRUPT_FRAME_ESTAT_OFFSET
    csrrd   $t0, LOONGARCH_CSR_BADV
    st.d    $t0, $sp, INTERRUPT_FRAME_BADV_OFFSET
    csrrd   $t0, LOONGARCH_CSR_PRMD
    st.d    $t0, $sp, INTERRUPT_FRAME_PRMD_OFFSET

    move    $a0, $sp
    bl      on_exception

    ld.d    $t1, $sp, INTERRUPT_FRAME_ERA_OFFSET
    csrwr   $t1, LOONGARCH_CSR_ERA
    ld.d    $t1, $sp, INTERRUPT_FRAME_PRMD_OFFSET
    csrwr   $t1, LOONGARCH_CSR_PRMD

    ld.d    $ra,   $sp, INTERRUPT_FRAME_GPR_OFFSET(1)
    ld.d    $tp,   $sp, INTERRUPT_FRAME_GPR_OFFSET(2)
    ld.d    $a0,   $sp, INTERRUPT_FRAME_GPR_OFFSET(4)
    ld.d    $a1,   $sp, INTERRUPT_FRAME_GPR_OFFSET(5)
    ld.d    $a2,   $sp, INTERRUPT_FRAME_GPR_OFFSET(6)
    ld.d    $a3,   $sp, INTERRUPT_FRAME_GPR_OFFSET(7)
    ld.d    $a4,   $sp, INTERRUPT_FRAME_GPR_OFFSET(8)
    ld.d    $a5,   $sp, INTERRUPT_FRAME_GPR_OFFSET(9)
    ld.d    $a6,   $sp, INTERRUPT_FRAME_GPR_OFFSET(10)
    ld.d    $a7,   $sp, INTERRUPT_FRAME_GPR_OFFSET(11)
    ld.d    $t1,   $sp, INTERRUPT_FRAME_GPR_OFFSET(13)
    ld.d    $t2,   $sp, INTERRUPT_FRAME_GPR_OFFSET(14)
    ld.d    $t3,   $sp, INTERRUPT_FRAME_GPR_OFFSET(15)
    ld.d    $t4,   $sp, INTERRUPT_FRAME_GPR_OFFSET(16)
    ld.d    $t5,   $sp, INTERRUPT_FRAME_GPR_OFFSET(17)
    ld.d    $t6,   $sp, INTERRUPT_FRAME_GPR_OFFSET(18)
    ld.d    $t7,   $sp, INTERRUPT_FRAME_GPR_OFFSET(19)
    ld.d    $t8,   $sp, INTERRUPT_FRAME_GPR_OFFSET(20)
    ld.d    $r21,  $sp, INTERRUPT_FRAME_GPR_OFFSET(21)
    ld.d    $fp,   $sp, INTERRUPT_FRAME_GPR_OFFSET(22)
    ld.d    $s0,   $sp, INTERRUPT_FRAME_GPR_OFFSET(23)
    ld.d    $s1,   $sp, INTERRUPT_FRAME_GPR_OFFSET(24)
    ld.d    $s2,   $sp, INTERRUPT_FRAME_GPR_OFFSET(25)
    ld.d    $s3,   $sp, INTERRUPT_FRAME_GPR_OFFSET(26)
    ld.d    $s4,   $sp, INTERRUPT_FRAME_GPR_OFFSET(27)
    ld.d    $s5,   $sp, INTERRUPT_FRAME_GPR_OFFSET(28)
    ld.d    $s6,   $sp, INTERRUPT_FRAME_GPR_OFFSET(29)
    ld.d    $s7,   $sp, INTERRUPT_FRAME_GPR_OFFSET(30)
    ld.d    $s8,   $sp, INTERRUPT_FRAME_GPR_OFFSET(31)

    ld.d    $sp, $sp, INTERRUPT_FRAME_GPR_OFFSET(3)
    ld.d    $t0, $sp, -INTERRUPT_FRAME_SIZE + INTERRUPT_FRAME_GPR_OFFSET(12)
    ertn
.size exception_entry, . - exception_entry
