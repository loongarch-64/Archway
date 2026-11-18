/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file loongarch.h
 * @brief LoongArch csrs register numbers.
 */

#ifndef ARCHWAY_ARCH_LOONGARCH_H
#define ARCHWAY_ARCH_LOONGARCH_H

#define LOONGARCH_CSR_CRMD 0x00
#define LOONGARCH_CSR_PRMD 0x01
#define LOONGARCH_CSR_EUEN 0x02
#define LOONGARCH_CSR_ECFG 0x04
#define LOONGARCH_CSR_ESTAT 0x05
#define LOONGARCH_CSR_ERA 0x06
#define LOONGARCH_CSR_BADV 0x07
#define LOONGARCH_CSR_EENTRY 0x0c
#define LOONGARCH_CSR_PRCFG1 0x21
#define LOONGARCH_CSR_TCFG 0x41
#define LOONGARCH_CSR_TICLR 0x44
#define LOONGARCH_CSR_PGDL 0x19
#define LOONGARCH_CSR_PGDH 0x1a
#define LOONGARCH_CSR_TLBRENTRY 0x88
#define LOONGARCH_CSR_TLBRBADV 0x89
#define LOONGARCH_CSR_TLBRERA 0x8a
#define LOONGARCH_CSR_TLBRSAVE 0x8b
#define LOONGARCH_CSR_TLBRELO0 0x8c
#define LOONGARCH_CSR_TLBRELO1 0x8d
#define LOONGARCH_CSR_TLBREHI 0x8e
#define LOONGARCH_CSR_PGD 0x1b
/* SAVE0/SAVE1 are reserved for TLB refill
   normal exception entry uses SAVE3 */
#define LOONGARCH_CSR_SAVE0 0x30
#define LOONGARCH_CSR_SAVE1 0x31
#define LOONGARCH_CSR_SAVE2 0x32
#define LOONGARCH_CSR_SAVE3 0x33

#define LOONGARCH_CRMD_IE (1 << 2)
#define LOONGARCH_INT_TIMER (1 << 11)

#define LOONGARCH_TLBREHI_PS_BITS 6
#define LOONGARCH_INVTLB_ALL 0

#define GPR_ZERO 0
#define GPR_RA 1
#define GPR_TP 2
#define GPR_SP 3
#define GPR_A0 4
#define GPR_A1 5
#define GPR_A2 6
#define GPR_A3 7
#define GPR_A4 8
#define GPR_A5 9
#define GPR_A6 10
#define GPR_A7 11
#define GPR_T0 12
#define GPR_T1 13
#define GPR_T2 14
#define GPR_T3 15
#define GPR_T4 16
#define GPR_T5 17
#define GPR_T6 18
#define GPR_T7 19
#define GPR_T8 20
#define GPR_R21 21
#define GPR_FP 22
#define GPR_S0 23
#define GPR_S1 24
#define GPR_S2 25
#define GPR_S3 26
#define GPR_S4 27
#define GPR_S5 28
#define GPR_S6 29
#define GPR_S7 30
#define GPR_S8 31

#endif /* ARCHWAY_ARCH_LOONGARCH_H */
