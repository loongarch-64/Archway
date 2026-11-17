/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file handoff.S
 * @brief Mapping setup and kernel entry transition.
 */

#include "loongarch.h"
#include "paging_layout.h"

.text
.global enter_kernel
.hidden enter_kernel
.type enter_kernel, @function
enter_kernel:
    /* a0 = page table root
       a1 = virtual entry
       a2 = HHDM pointer to struct archway_boot_info
    */
    li.d $t0, LOONGARCH_CRMD_IE
    csrxchg $zero, $t0, LOONGARCH_CSR_CRMD
    csrwr $zero, LOONGARCH_CSR_ECFG
    csrwr $zero, LOONGARCH_CSR_TCFG
    li.d $t0, (LOONGARCH_DMW_PLV0 | LOONGARCH_DMW_COHERENT) /* temporary cached identity mapping. */
    csrwr $t0, LOONGARCH_CSR_DMW0
    li.d $t0, (ARCHWAY_HHDM_BASE | LOONGARCH_DMW_PLV0 | LOONGARCH_DMW_COHERENT) /* cached PLV0 HHDM. */
    csrwr $t0, LOONGARCH_CSR_DMW1
    /* strongly ordered, uncached PLV0 window for device MMIO. */
    li.d $t0, (ARCHWAY_MMIO_BASE | LOONGARCH_DMW_PLV0)
    csrwr $t0, LOONGARCH_CSR_DMW2
    csrwr $zero, LOONGARCH_CSR_DMW3

    csrwr $zero, LOONGARCH_CSR_TLBRERA
    csrwr $zero, LOONGARCH_CSR_ASID
    csrwr $zero, LOONGARCH_CSR_PGDL /* no user mappings. */
    csrwr $a0, LOONGARCH_CSR_PGDH /* four-level kernel page table. */
    csrwr $zero, LOONGARCH_CSR_PWCL /* manual refill. software walker. */
    csrwr $zero, LOONGARCH_CSR_PWCH
    li.d $t0, PAGE_SHIFT
    csrwr $t0, LOONGARCH_CSR_STLBPS /* 4 KiB STLB pages */
    la.local $t0, tlb_refill
    bstrpick.d $t0, $t0, (PHYSICAL_BITS - 1), 0
    csrwr $t0, LOONGARCH_CSR_TLBRENTRY
    la.local $t0, bootstrap_fault
    bstrpick.d $t0, $t0, (PHYSICAL_BITS - 1), 0
    li.d $t1, ARCHWAY_HHDM_BASE
    or $t0, $t0, $t1
    csrwr $t0, LOONGARCH_CSR_EENTRY
    invtlb LOONGARCH_INVTLB_ALL, $zero, $zero
    dbar LOONGARCH_BARRIER_FULL
    ibar LOONGARCH_BARRIER_FULL
    li.d $t0, (LOONGARCH_CRMD_PG | LOONGARCH_CRMD_DATF_COHERENT | LOONGARCH_CRMD_DATM_COHERENT) /* PLV0. interrupts off. mapped addressing. */
    csrwr $t0, LOONGARCH_CSR_CRMD
    la.local $t0, .Lhigh
    bstrpick.d $t0, $t0, (PHYSICAL_BITS - 1), 0
    or $t0, $t0, $t1
    jr $t0
.Lhigh:
    csrwr $zero, LOONGARCH_CSR_DMW0
    move $ra, $zero
    move $a0, $a2
    jr $a1
.size enter_kernel, . - enter_kernel

.balign PAGE_SIZE
.type tlb_refill, @function
tlb_refill:
    csrwr $t0, LOONGARCH_CSR_TLBRSAVE
    csrwr $t1, LOONGARCH_CSR_SAVE0
    csrwr $t2, LOONGARCH_CSR_SAVE1
    csrrd $t0, LOONGARCH_CSR_TLBRBADV
    bge $t0, $zero, .Lrefill_fault
    csrrd $t0, LOONGARCH_CSR_PGDH
    csrrd $t1, LOONGARCH_CSR_TLBRBADV
    bstrpick.d $t1, $t1, (PT_L3_SHIFT + PT_INDEX_BITS - 1), PT_L3_SHIFT
    slli.d $t1, $t1, PT_ENTRY_SHIFT
    ldx.d $t0, $t0, $t1
    beqz $t0, .Lrefill_fault
    csrrd $t1, LOONGARCH_CSR_TLBRBADV
    bstrpick.d $t1, $t1, (PT_L2_SHIFT + PT_INDEX_BITS - 1), PT_L2_SHIFT
    slli.d $t1, $t1, PT_ENTRY_SHIFT
    ldx.d $t0, $t0, $t1
    beqz $t0, .Lrefill_fault
    csrrd $t1, LOONGARCH_CSR_TLBRBADV
    bstrpick.d $t1, $t1, (PT_L1_SHIFT + PT_INDEX_BITS - 1), PT_L1_SHIFT
    slli.d $t1, $t1, PT_ENTRY_SHIFT
    ldx.d $t0, $t0, $t1
    beqz $t0, .Lrefill_fault
    csrrd $t1, LOONGARCH_CSR_TLBRBADV
    bstrpick.d $t1, $t1, (PT_L1_SHIFT - 1), TLB_PAIR_SHIFT
    slli.d $t1, $t1, TLB_PAIR_ENTRY_SHIFT
    add.d $t0, $t0, $t1
    ld.d $t1, $t0, 0
    ld.d $t2, $t0, PT_ENTRY_SIZE
    csrwr $t1, LOONGARCH_CSR_TLBRELO0
    csrwr $t2, LOONGARCH_CSR_TLBRELO1
    csrrd $t0, LOONGARCH_CSR_TLBREHI /* retain the fault VPPN. */
    bstrins.d $t0, $zero, (LOONGARCH_TLBREHI_PS_BITS - 1), 0
    ori $t0, $t0, PAGE_SHIFT
    csrwr $t0, LOONGARCH_CSR_TLBREHI
    tlbfill
    csrrd $t2, LOONGARCH_CSR_SAVE1
    csrrd $t1, LOONGARCH_CSR_SAVE0
    csrrd $t0, LOONGARCH_CSR_TLBRSAVE
    ertn
.Lrefill_fault:
    b .Lrefill_fault
.size tlb_refill, . - tlb_refill

.balign LOONGARCH_EXCEPTION_ALIGNMENT
.type bootstrap_fault, @function
bootstrap_fault:
    b bootstrap_fault
.size bootstrap_fault, . - bootstrap_fault
