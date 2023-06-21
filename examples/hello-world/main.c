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
#include "pmp.h"

int main(void)
{
    int buffer[4] = {0, 1, 2, 3};
    puts("Hello World!");
    write_pmpaddr(0, make_napot((uintptr_t) buffer, sizeof(buffer)));
    set_pmpcfg(0, PMP_NAPOT | PMP_R);
    for (int i = 0; i < 16; i++)
    {
        print_pmpcfg(i);
    }
    
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    for (int i = 0; i < 4; i++)
    {
        buffer[i] = 0xAA;
    }
    return 0;
}
