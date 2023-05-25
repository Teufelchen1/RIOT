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

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "pmp.h"

int main(void)
{
    puts("Hello World!");

    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    //set_pmpXcfg(0, 0x20000000, PMP_L | PMP_TOR);
    //set_pmpXcfg(1, 0x28000000, PMP_L | PMP_TOR | PMP_R | PMP_W | PMP_X);
    //set_pmpXcfg(2, 0x3c000000, PMP_L | PMP_TOR);
    //set_pmpXcfg(3, 0x3fc80000, PMP_L | PMP_TOR | PMP_R);
    //int test_var = 12;
    //set_pmpXcfg(3, (uintptr_t) &test_var, PMP_L | PMP_NA4 | PMP_R);

    set_pmpXcfg(6, make_napot(0x80000000, 64), PMP_L | PMP_NAPOT | PMP_R | PMP_W);

    for (int i = 0; i < 4; i++)
    {
        printf("pmpcfg%d\n", i);
        for (int k = 0; k < 4; k++)
        {
            print_pmpXcfg((i*4) + k);
        }
    }

    //test_var = 1;
    
    return 0;
}
