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
 * @brief       IPC pingpong application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

#include "periph/gpio.h"
#include "thread.h"
#include "msg.h"

void waste_time(void) {
    puts("Wasting time...");
    for (size_t i = 0; i < 255; i++)
    {
        putc(' ', stdout);
    }   
}

void *second_thread(void *arg)
{
    (void) arg;

    printf("2nd thread started, pid: %" PRIkernel_pid "\n", thread_getpid());

    while (1) {
        gpio_set(GPIO_PIN(0, 22));
        //waste_time();
        thread_yield();
    }

    return NULL;
}

char second_thread_stack[THREAD_STACKSIZE_MAIN];

int main(void)
{
    printf("Starting IPC Ping-pong example...\n");
    gpio_init(GPIO_PIN(0, 22), GPIO_OUT);
    printf("1st thread started, pid: %" PRIkernel_pid "\n", thread_getpid());
    printf("Stack of 2nd thread: %08X - %08X\n", (uintptr_t) second_thread_stack, (uintptr_t) (&second_thread_stack[THREAD_STACKSIZE_MAIN-1]));

    thread_create(second_thread_stack, sizeof(second_thread_stack), 
                  THREAD_PRIORITY_MAIN, THREAD_CREATE_STACKTEST,
                  second_thread, NULL, "pong");

    while (1) {
        gpio_clear(GPIO_PIN(0, 22));
        //waste_time();
        thread_yield();
    }
}
