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

#include "thread.h"
#include "msg.h"

static inline void ecall_dispatch(uint32_t num, void *ctx)
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

static inline __attribute__((always_inline)) unsigned int _irq_disable(void)
{
    unsigned int value = 0;
    ecall_dispatch(2, &value);
    return value;
}

static inline __attribute__((always_inline)) unsigned int _irq_enable(void)
{
    unsigned int value = 0;
    ecall_dispatch(2, &value);
    return value;
}

typedef struct {
    char *stack;
    int stacksize;
    uint8_t priority;
    int flags;
    thread_task_func_t function;
    void *arg;
    const char *name;
    kernel_pid_t pid;
} tc_args;

kernel_pid_t thread_create_ecall(char *stack, int stacksize, uint8_t priority,
                    int flags, thread_task_func_t function, void *arg,
                    const char *name) {
    tc_args args;
    args.stack = stack;
    args.stacksize = stacksize;
    args.priority = priority;
    args.flags = flags;
    args.function = function;
    args.arg = arg;
    args.name = name;
    ecall_dispatch(5, &args);
    return args.pid;
}

void thread_yield_ecall(void) {
    ecall_dispatch(5, NULL);
}

void *second_thread(void *arg)
{
    (void) arg;

    printf("2nd thread started, pid: %" PRIkernel_pid "\n", thread_getpid());
    //msg_t m;

    while (1) {
        puts("Hello from second_thread");
        thread_yield_ecall();
        // msg_receive(&m);
        // printf("2nd: Got msg from %" PRIkernel_pid "\n", m.sender_pid);
        // m.content.value++;
        // msg_reply(&m, &m);
    }

    return NULL;
}

char second_thread_stack[THREAD_STACKSIZE_MAIN];

int main(void)
{
    printf("Starting IPC Ping-pong example...\n");
    unsigned int state = _irq_enable();
    //ecall_dispatch(4, &state);
    printf("State is now: %d\n", state);
    state = _irq_disable();
    printf("State is now: %d\n", state);
    printf("1st thread started, pid: %" PRIkernel_pid "\n", thread_getpid());

    kernel_pid_t pid = thread_create_ecall(second_thread_stack, sizeof(second_thread_stack),
                            THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST | THREAD_CREATE_SLEEPING,
                            second_thread, NULL, "pong");
    printf("Created thread %" PRIkernel_pid "\n", pid);
    //m.content.value = 1;

    while (1) {
        puts("Hello from main()");
        thread_yield_ecall();
        //printf("1st: Got msg with content %u\n", (unsigned int)m.content.value);
    }
}
