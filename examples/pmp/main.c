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
#include <stdint.h>
#include "vendor/pmp.h"

void print_all_pmp_entries(void) {
    for (int i = 0; i < 16; i++)
    {
        print_pmpXcfg(i);
    }
}

static uint32_t test_var = 0xff;

int main(void)
{
    puts("Hello World!");

    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    print_all_pmp_entries();
    set_pmpXcfg(13, &test_var, sizeof(test_var), PMP_R | PMP_W); 
    print_all_pmp_entries();

    return 0;
}
