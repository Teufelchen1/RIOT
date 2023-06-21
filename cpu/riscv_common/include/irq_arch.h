/*
 * Copyright (C) 2017 Ken Rabold
 *               2020 Inria
 *               2020 Otto-von-Guericke-Universität Magdeburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup         cpu_riscv_common
 * @{
 *
 * @file
 * @brief           Implementation of the kernels irq interface
 *
 * @author          Ken Rabold
 * @author          Alexandre Abadie <alexandre.abadie@inria.fr>
 * @author          Marian Buschsieweke <marian.buschsieweke@ovgu.de>
 */

#ifndef IRQ_ARCH_H
#define IRQ_ARCH_H

#include <stddef.h>
#include <stdint.h>

#include "irq.h"
#include "vendor/riscv_csr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Bit mask for the MCAUSE register
 */
#define CPU_CSR_MCAUSE_CAUSE_MSK        (0x0fffu)

extern volatile int riscv_in_isr;

static inline void __ecall_dispatch(uint32_t num, void *ctx)
{
    /* function arguments are in a0 and a1 as per ABI */
    __asm__ volatile (
        "add a0, x0, %[num] \n"
        "add a1, x0, %[ctx] \n"
        "ECALL\n"
        : /* No outputs */
        :[num] "r" (num), [ctx] "r" (ctx)
        : "memory", "a0", "a1"
        );
}

/**
 * @brief Enable all maskable interrupts
 */
static inline __attribute__((always_inline)) unsigned int irq_enable(void)
{
    unsigned int value = 0;
    //__ecall_dispatch(1, &value);
    /* Enable all interrupts */
    
    unsigned state;

    __asm__ volatile (
        "csrrs %[dest], mstatus, %[mask]"
        :[dest]    "=r" (state)
        :[mask]    "i" (MSTATUS_MIE)
        : "memory"
        );
    return state;
    
   return value;
}

/**
 * @brief Disable all maskable interrupts
 */
static inline __attribute__((always_inline)) unsigned int irq_disable(void)
{
   // unsigned int value = 0;
    //__ecall_dispatch(2, &value);
    
    unsigned int state;

    __asm__ volatile (
        "csrrc %[dest], mstatus, %[mask]"
        :[dest]    "=r" (state)
        :[mask]    "i" (MSTATUS_MIE)
        : "memory"
        );

    return state;
    
   //return value;
}

/**
 * @brief Restore the state of the IRQ flags
 */
static inline __attribute__((always_inline)) void irq_restore(
    unsigned int state)
{
    /* Restore all interrupts to given state */
    //(void) state;
    //__ecall_dispatch(3, &state);

    __asm__ volatile (
       "csrw mstatus, %[state]"
       : /* no outputs */
       :[state]   "r" (state)
       : "memory"
       );
}

/**
 * @brief See if the current context is inside an ISR
 */
static inline __attribute__((always_inline)) bool irq_is_in(void)
{
    return riscv_in_isr;
}

static inline __attribute__((always_inline)) bool irq_is_enabled(void)
{
    unsigned state;

    __asm__ volatile (
        "csrr %[dest], mstatus"
        :[dest]    "=r" (state)
        : /* no inputs */
        : "memory"
        );
    return (state & MSTATUS_MIE);
}

#ifdef __cplusplus
}
#endif

#endif /* IRQ_ARCH_H */
/** @} */
