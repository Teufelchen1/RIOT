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

#include "thread_arch.h"
#include "pmp.h"

int main(void)
{
    puts("Hello World!");

    thread_drop_privilege();

    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    thread_gain_privilege();
    for (uint8_t i = 0; i < 8; i++)
    {
        print_pmpcfg(i);
    }
    thread_drop_privilege();
    *(uint32_t *) 0x80000190 = 0x00;
    thread_gain_privilege();
    for (uint8_t i = 0; i < 8; i++)
    {
        print_pmpcfg(i);
    }
    return 0;
}
