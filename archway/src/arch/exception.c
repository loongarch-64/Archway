/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file exception.c
 * @brief Initial exception receiver.
 */

#include <arch/hcf.h>
#include <arch/interrupt_frame.h>
#include <arch/loongarch.h>
#include <log.h>

#define LOONGARCH_ESTAT_ECODE_SHIFT 16
#define LOONGARCH_ESTAT_ECODE_MASK 0x3f
#define LOONGARCH_ESTAT_ESUBCODE_SHIFT 22
#define LOONGARCH_ESTAT_ESUBCODE_MASK 0x1ff

void
on_exception(struct interrupt_frame *frame) {
    uint64_t ecode = (frame->estat >> LOONGARCH_ESTAT_ECODE_SHIFT) & LOONGARCH_ESTAT_ECODE_MASK;
    uint64_t esubcode =
        (frame->estat >> LOONGARCH_ESTAT_ESUBCODE_SHIFT) & LOONGARCH_ESTAT_ESUBCODE_MASK;

    kprintf("!!! Exception !!!\n");
    kprintf("ECODE=%02llX ESUBCODE=%03llX\n", ecode, esubcode);
    kprintf(
        "ERA=%016llX BADV=%016llX PRMD=%016llX ESTAT=%016llX\n",
        frame->era,
        frame->badv,
        frame->prmd,
        frame->estat
    );
    kprintf(
        "ZERO=%016llX RA=%016llX TP=%016llX SP=%016llX\n",
        frame->gpr[GPR_ZERO],
        frame->gpr[GPR_RA],
        frame->gpr[GPR_TP],
        frame->gpr[GPR_SP]
    );
    kprintf(
        "A0=%016llX A1=%016llX A2=%016llX A3=%016llX\n",
        frame->gpr[GPR_A0],
        frame->gpr[GPR_A1],
        frame->gpr[GPR_A2],
        frame->gpr[GPR_A3]
    );
    kprintf(
        "A4=%016llX A5=%016llX A6=%016llX A7=%016llX\n",
        frame->gpr[GPR_A4],
        frame->gpr[GPR_A5],
        frame->gpr[GPR_A6],
        frame->gpr[GPR_A7]
    );
    kprintf(
        "T0=%016llX T1=%016llX T2=%016llX T3=%016llX\n",
        frame->gpr[GPR_T0],
        frame->gpr[GPR_T1],
        frame->gpr[GPR_T2],
        frame->gpr[GPR_T3]
    );
    kprintf(
        "T4=%016llX T5=%016llX T6=%016llX T7=%016llX\n",
        frame->gpr[GPR_T4],
        frame->gpr[GPR_T5],
        frame->gpr[GPR_T6],
        frame->gpr[GPR_T7]
    );
    kprintf(
        "T8=%016llX R21=%016llX FP=%016llX S0=%016llX\n",
        frame->gpr[GPR_T8],
        frame->gpr[GPR_R21],
        frame->gpr[GPR_FP],
        frame->gpr[GPR_S0]
    );
    kprintf(
        "S1=%016llX S2=%016llX S3=%016llX S4=%016llX\n",
        frame->gpr[GPR_S1],
        frame->gpr[GPR_S2],
        frame->gpr[GPR_S3],
        frame->gpr[GPR_S4]
    );
    kprintf(
        "S5=%016llX S6=%016llX S7=%016llX S8=%016llX\n",
        frame->gpr[GPR_S5],
        frame->gpr[GPR_S6],
        frame->gpr[GPR_S7],
        frame->gpr[GPR_S8]
    );

    if (ecode == 0x0c) {
        frame->era += 4;
        return;
    }

    hcf();
}
