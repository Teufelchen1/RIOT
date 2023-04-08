/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include "riscv/csr.h"
#include "syscalls.h"

#define CONFIG_ESP_SYSTEM_PANIC_PRINT_HALT
#define ESP_LOG_VERBOSE
#define CONFIG_APP_BUILD_BOOTLOADER 1

#define SIZE 16

uint8_t volatile * reg = (uint8_t volatile (*))(0x3A0);

uint32_t read_pmpaddr(int num) {
    uint32_t value = 0;
    switch(num) {
        case 0:
            value = RV_READ_CSR(CSR_PMPADDR0);
            break;
        case 1:
            value = RV_READ_CSR(CSR_PMPADDR0 + 1);
            break;
        case 2:
            value = RV_READ_CSR(CSR_PMPADDR0 + 2);
            break;
        case 3:
            value = RV_READ_CSR(CSR_PMPADDR0 + 3);
            break;
        case 4:
            value = RV_READ_CSR(CSR_PMPADDR0 + 4);
            break;
        case 5:
            value = RV_READ_CSR(CSR_PMPADDR0 + 5);
            break;
        case 6:
            value = RV_READ_CSR(CSR_PMPADDR0 + 6);
            break;
        case 7:
            value = RV_READ_CSR(CSR_PMPADDR0 + 7);
            break;
        case 8:
            value = RV_READ_CSR(CSR_PMPADDR0 + 8);
            break;
        case 9:
            value = RV_READ_CSR(CSR_PMPADDR0 + 9);
            break;
        case 10:
            value = RV_READ_CSR(CSR_PMPADDR0 + 10);
            break;
        case 11:
            value = RV_READ_CSR(CSR_PMPADDR0 + 11);
            break;
        case 12:
            value = RV_READ_CSR(CSR_PMPADDR0 + 12);
            break;
        case 13:
            value = RV_READ_CSR(CSR_PMPADDR0 + 13);
            break;
        case 14:
            value = RV_READ_CSR(CSR_PMPADDR0 + 14);
            break;
        case 15:
            value = RV_READ_CSR(CSR_PMPADDR0 + 15);
            break;
    }
    return value << PMP_SHIFT;
}

void print_pmp_reg(int num) {
    uint32_t value = 0;
    switch(num / 4) {
        case 0:
            value = RV_READ_CSR(CSR_PMPCFG0);
            break;
        case 1:
            value = RV_READ_CSR(CSR_PMPCFG0 + 1);
            break;
        case 2:
            value = RV_READ_CSR(CSR_PMPCFG0 + 2);
            break;
        case 3:
            value = RV_READ_CSR(CSR_PMPCFG0 + 3);
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

int main(void)
{
    unsigned char memory[SIZE];
    puts("Hello World!");
    system_wdt_stop();
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);
    printf("Memory address start: 0x%08lX, stopp: 0x%08lX\n", (uint32_t) memory, (uint32_t) &memory[SIZE]);
    //for(int i = 0; i < 16; i++)
    //    print_pmp_reg(i);
    for (int i = 0; i < SIZE; i++)
    {
        memory[i] = 0;
    }

    /* 0x00000000 - SOC_DEBUG_LOW: Denied */
    PMP_ENTRY_SET(0, 0x20000000, PMP_TOR);

    /* SOC_DEBUG_LOW - SOC_DEBUG_HIGH: RWX */
    PMP_ENTRY_SET(1, 0x28000000, PMP_TOR | PMP_R | PMP_W | PMP_X);

    /* SOC_DEBUG_HIGH - SOC_DROM_LOW: Denied */
    PMP_ENTRY_SET(2, 0x3C000000, PMP_TOR);

    /* SOC_DROM_LOW - SOC_DRAM_LOW: R */
    PMP_ENTRY_SET(3, 0x3FC80000, PMP_TOR | PMP_R);

    PMP_ENTRY_SET(4, (int) memory, PMP_TOR | PMP_R | PMP_W);

    PMP_ENTRY_SET(5, (int) &memory[SIZE], PMP_TOR | PMP_R);

    /* SOC_DRAM_LOW - SOC_DRAM_HIGH: RW */
    PMP_ENTRY_SET(6, 0x3FCE0000, PMP_TOR | PMP_R | PMP_W);

    print_pmp_reg(5);
    //for(int i = 0; i < 16; i++)
    //    print_pmp_reg(i);

    for (int i = 0; i < SIZE; i++)
    {
        unsigned char old = memory[i];
        memory[i] = 0xaa;
        unsigned char new = memory[i];
        printf("%08lX|%04d| %x, %x\n", (uint32_t) &memory[i], i, old, new);
    }

    puts("");
    PMP_ENTRY_SET(5, (int) &memory[SIZE], PMP_L | PMP_TOR | PMP_R);
    puts("Locking for machine mode");
    print_pmp_reg(5);
    for (int i = 0; i < 1; i++)
    {
        unsigned char old = memory[i];
        memory[i] = 0xbb;
        unsigned char new = memory[i];
        printf("%08lX|%04d| %x, %x\n", (uint32_t) &memory[i], i, old, new);
    }
    
    printf("hello?\n");
    for(int i = 0; i < 16; i++)
        print_pmp_reg(i);
    return 0;
}
