/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file paging.S
 * @brief LoongArch page table access and TLB helpers.
 */

#include "loongarch.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define PT_INDEX_BITS 9
#define PT_ENTRY_SHIFT 3
#define PT_ENTRY_SIZE (1 << PT_ENTRY_SHIFT)
#define PT_L1_SHIFT (PAGE_SHIFT + PT_INDEX_BITS)
#define PT_L2_SHIFT (PT_L1_SHIFT + PT_INDEX_BITS)
#define PT_L3_SHIFT (PT_L2_SHIFT + PT_INDEX_BITS)
#define TLB_PAIR_SHIFT (PAGE_SHIFT + 1)
#define TLB_PAIR_ENTRY_SHIFT (PT_ENTRY_SHIFT + 1)

.text
.align 2
.global paging_root_physical
.type paging_root_physical, @function
paging_root_physical:
    bstrpick.d $t0, $a0, 47, 47
    beqz $t0, 1f
    csrrd $a0, LOONGARCH_CSR_PGDH
    jr $ra
1:
    csrrd $a0, LOONGARCH_CSR_PGDL
    jr $ra
.size paging_root_physical, . - paging_root_physical

.align 2
.global paging_install_low_root
.type paging_install_low_root, @function
paging_install_low_root:
    csrwr $a0, LOONGARCH_CSR_PGDL
    invtlb LOONGARCH_INVTLB_ALL, $zero, $zero
    dbar 0
    ibar 0
    jr $ra
.size paging_install_low_root, . - paging_install_low_root

.align 2
.global paging_install_refill
.type paging_install_refill, @function
paging_install_refill:
    csrwr $a0, LOONGARCH_CSR_TLBRENTRY
    invtlb LOONGARCH_INVTLB_ALL, $zero, $zero
    dbar 0
    ibar 0
    jr $ra
.size paging_install_refill, . - paging_install_refill

.align 2
.global paging_flush_tlb
.type paging_flush_tlb, @function
paging_flush_tlb:
    invtlb 0, $zero, $zero
    dbar 0
    ibar 0
    jr $ra
.size paging_flush_tlb, . - paging_flush_tlb

.section .text.paging_refill, "ax", @progbits
.balign PAGE_SIZE
.global paging_refill_entry
.hidden paging_refill_entry
.type paging_refill_entry, @function
paging_refill_entry:
    csrwr $t0, LOONGARCH_CSR_TLBRSAVE
    csrwr $t1, LOONGARCH_CSR_SAVE0
    csrwr $t2, LOONGARCH_CSR_SAVE1
    /* PGD follows TLBRBADV and selects PGDL or PGDH for the refill */
    csrrd $t0, LOONGARCH_CSR_PGD
    beqz $t0, .Lrefill_fault
    csrrd $t1, LOONGARCH_CSR_TLBRBADV
    bstrpick.d $t1, $t1, (PT_L3_SHIFT + PT_INDEX_BITS - 1), PT_L3_SHIFT
    slli.d $t1, $t1, PT_ENTRY_SHIFT
    ldx.d $t0, $t0, $t1
    beqz $t0, .Lrefill_fault
    andi $t2, $t0, 0x40 /* directory bit 6 marks an unsupported huge page */
    beqz $t2, 1f
    b .Lrefill_fault
1:
    /* check HUGE and then strip PDE_OWNED 
       and all other low directory bits */
    bstrins.d $t0, $zero, (PAGE_SHIFT - 1), 0
    beqz $t0, .Lrefill_fault
    csrrd $t1, LOONGARCH_CSR_TLBRBADV
    bstrpick.d $t1, $t1, (PT_L2_SHIFT + PT_INDEX_BITS - 1), PT_L2_SHIFT
    slli.d $t1, $t1, PT_ENTRY_SHIFT
    ldx.d $t0, $t0, $t1
    beqz $t0, .Lrefill_fault
    andi $t2, $t0, 0x40
    beqz $t2, 1f
    b .Lrefill_fault
1:
    bstrins.d $t0, $zero, (PAGE_SHIFT - 1), 0
    beqz $t0, .Lrefill_fault
    csrrd $t1, LOONGARCH_CSR_TLBRBADV
    bstrpick.d $t1, $t1, (PT_L1_SHIFT + PT_INDEX_BITS - 1), PT_L1_SHIFT
    slli.d $t1, $t1, PT_ENTRY_SHIFT
    ldx.d $t0, $t0, $t1
    beqz $t0, .Lrefill_fault
    andi $t2, $t0, 0x40
    beqz $t2, 1f
    b .Lrefill_fault
1:
    bstrins.d $t0, $zero, (PAGE_SHIFT - 1), 0
    beqz $t0, .Lrefill_fault
    csrrd $t1, LOONGARCH_CSR_TLBRBADV
    bstrpick.d $t1, $t1, (PT_L1_SHIFT - 1), TLB_PAIR_SHIFT
    slli.d $t1, $t1, TLB_PAIR_ENTRY_SHIFT
    add.d $t0, $t0, $t1
    ld.d $t1, $t0, 0
    ld.d $t2, $t0, PT_ENTRY_SIZE
    csrwr $t1, LOONGARCH_CSR_TLBRELO0
    csrwr $t2, LOONGARCH_CSR_TLBRELO1
    csrrd $t0, LOONGARCH_CSR_TLBREHI /* retain the fault VPPN */
    bstrins.d $t0, $zero, (LOONGARCH_TLBREHI_PS_BITS - 1), 0
    ori $t0, $t0, PAGE_SHIFT
    csrwr $t0, LOONGARCH_CSR_TLBREHI
    tlbfill
    csrrd $t2, LOONGARCH_CSR_SAVE1
    csrrd $t1, LOONGARCH_CSR_SAVE0
    csrrd $t0, LOONGARCH_CSR_TLBRSAVE
    ertn
.Lrefill_fault:
    /* install an invalid translation 
       so ERTN retries into the normal 
       page-fault path */
    csrwr $zero, LOONGARCH_CSR_TLBRELO0
    csrwr $zero, LOONGARCH_CSR_TLBRELO1
    csrrd $t0, LOONGARCH_CSR_TLBREHI
    bstrins.d $t0, $zero, (LOONGARCH_TLBREHI_PS_BITS - 1), 0
    ori $t0, $t0, PAGE_SHIFT
    csrwr $t0, LOONGARCH_CSR_TLBREHI
    tlbfill
    csrrd $t2, LOONGARCH_CSR_SAVE1
    csrrd $t1, LOONGARCH_CSR_SAVE0
    csrrd $t0, LOONGARCH_CSR_TLBRSAVE
    ertn
.size paging_refill_entry, . - paging_refill_entry
