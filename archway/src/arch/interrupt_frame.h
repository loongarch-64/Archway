/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file interrupt_frame.h
 * @brief Register and exception state.
 */

#ifndef ARCHWAY_ARCH_INTERRUPT_FRAME_H
#define ARCHWAY_ARCH_INTERRUPT_FRAME_H

#define INTERRUPT_FRAME_GPR_OFFSET(reg) ((reg) * 8)
#define INTERRUPT_FRAME_ERA_OFFSET      (32 * 8)
#define INTERRUPT_FRAME_ESTAT_OFFSET    (33 * 8)
#define INTERRUPT_FRAME_BADV_OFFSET     (34 * 8)
#define INTERRUPT_FRAME_PRMD_OFFSET     (35 * 8)
#define INTERRUPT_FRAME_SIZE            (36 * 8)

#ifndef __ASSEMBLER__
#include <stdint.h>

struct interrupt_frame {
    uint64_t gpr[32];
    uint64_t era;   // exception return address
    uint64_t estat; // exception status
    uint64_t badv;  // faulting virtual address
    uint64_t prmd;  // previous mode state
};

void exception_init(void);
void on_exception(struct interrupt_frame *frame);
#endif

#endif /* ARCHWAY_ARCH_INTERRUPT_FRAME_H */
