/*
 * Copyright (C) 2016 Loci Controls Inc.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_cortexm_common
 * @{
 *
 * @file        mpu.c
 * @brief       Cortex-M Memory Protection Unit (MPU) Driver
 *
 * @author      Ian Martin <ian@locicontrols.com>
 *
 * @}
 */

#include "cpu.h"
#include "vendor/riscv_csr.h"
#include <stdio.h>
#include <string.h>

uint32_t read_pmpaddr(int num) {
    uint32_t value = 0;
    switch(num) {
        case 0:
            value = read_csr(CSR_PMPADDR0);
            break;
        case 1:
            value = read_csr(CSR_PMPADDR0 + 1);
            break;
        case 2:
            value = read_csr(CSR_PMPADDR0 + 2);
            break;
        case 3:
            value = read_csr(CSR_PMPADDR0 + 3);
            break;
        case 4:
            value = read_csr(CSR_PMPADDR0 + 4);
            break;
        case 5:
            value = read_csr(CSR_PMPADDR0 + 5);
            break;
        case 6:
            value = read_csr(CSR_PMPADDR0 + 6);
            break;
        case 7:
            value = read_csr(CSR_PMPADDR0 + 7);
            break;
        case 8:
            value = read_csr(CSR_PMPADDR0 + 8);
            break;
        case 9:
            value = read_csr(CSR_PMPADDR0 + 9);
            break;
        case 10:
            value = read_csr(CSR_PMPADDR0 + 10);
            break;
        case 11:
            value = read_csr(CSR_PMPADDR0 + 11);
            break;
        case 12:
            value = read_csr(CSR_PMPADDR0 + 12);
            break;
        case 13:
            value = read_csr(CSR_PMPADDR0 + 13);
            break;
        case 14:
            value = read_csr(CSR_PMPADDR0 + 14);
            break;
        case 15:
            value = read_csr(CSR_PMPADDR0 + 15);
            break;
    }
    return value << PMP_SHIFT;
}

void print_pmp_reg(int num) {
    uint32_t value = 0;
    switch(num / 4) {
        case 0:
            value = read_csr(CSR_PMPCFG0);
            break;
        case 1:
            value = read_csr(CSR_PMPCFG0 + 1);
            break;
        case 2:
            value = read_csr(CSR_PMPCFG0 + 2);
            break;
        case 3:
            value = read_csr(CSR_PMPCFG0 + 3);
            break;
    }
    value = value >> (num % 4) * 8;
    char L = '-';
    char X = '-';
    char W = '-';
    char R = '-';
    if (value & 0b10000000) {
        L = 'L';
    }
    if (value & 0b00000100) {
        X = 'X';
    }
    if (value & 0b00000010) {
        W = 'W';
    }
    if (value & 0b10000001) {
        R = 'R';
    }
    char *A = "OFF";
    switch((value & 0b00011000) >> 3) {
        case 0:
            A = "OFF";
            break;
        case 1:
            A = "TOR";
            break;
        case 2:
            A = "NA4";
            break;
        case 3:
            A = "NAPOT";
            break;
    }
    uint32_t addr = read_pmpaddr(num);
    uint32_t lower_addr = 0;
    if (strcmp(A, "OFF") == 0) {
        lower_addr = addr;
    } else if (strcmp(A, "TOR") == 0) {
        if (num > 0) {
            lower_addr = read_pmpaddr(num-1);
        } else {
            lower_addr = 0;
        }
    } else if (strcmp(A, "NA4") == 0) {
        lower_addr = addr;
    } else {
        uint32_t size = 8;
        for (int i = 0; (addr >> (PMP_SHIFT + i)) & 0b01; i++) {
            size = size << 1;
        }
        lower_addr = addr + size;
    }
    printf("pmp%02dcfg: %c00%s%c%c%c\t0x%08lX - 0x%08lX\n", num, L, A, X, W, R, lower_addr, addr);
}

int pmp_disable(void) {
#if __PMP_PRESENT
    /* PMP is always enabled. */
    return 0;
#else
    return -1;
#endif
}

int pmp_enable(void) {
#if __PMP_PRESENT
    /* PMP is always enabled. */
    return 0;
#else
    return -1;
#endif
}

bool pmp_enabled(void) {
#if __PMP_PRESENT
    /* PMP is always enabled. */
    return true;
#else
    return false;
#endif
}

int pmp_configure(uint_fast8_t region, uintptr_t base, uint_fast32_t attr) {
#if __PMP_PRESENT
switch (region) {
    case 0:
        PMP_ENTRY_SET(0, base, attr);
        break;
    case 1:
        PMP_ENTRY_SET(1, base, attr);
        break;
    case 2:
        PMP_ENTRY_SET(2, base, attr);
        break;
    case 3:
        PMP_ENTRY_SET(3, base, attr);
        break;
    case 4:
        PMP_ENTRY_SET(4, base, attr);
        break;
    default:
        PMP_ENTRY_SET(3, base, attr);
        break;
}
    return 0;
#else
    (void)region;
    (void)base;
    (void)attr;
    return -1;
#endif
}
