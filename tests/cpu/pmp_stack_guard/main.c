/*
 * Copyright (C) 2016 Loci Controls Inc.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief Test application for the pmp_stack_guard pseudo-module
 *
 * @author Ian Martin <ian@locicontrols.com>
 *
 * @}
 */

#include <stdio.h>

#include "cpu.h"
#include "thread.h"
#include "thread_arch.h"
#include "pmp.h"

/* RIOT's MPU headers gracefully fail when no MPU is present.
 * Use this to catch if RIOT's features are correctly gating MPU use.
 */
//#if !__MPU_PRESENT
//#error "(!__MPU_PRESENT)"
//#endif

#define CANARY_VALUE 0xdeadbeef

static struct {
    unsigned int canary;
    char stack[THREAD_STACKSIZE_MAIN];
} buf;

char bugger[128];

static inline unsigned int __get_PSP(void) {
    unsigned int __tmp;
    __asm__ volatile ("mv %0, sp" : "=r"(__tmp));
    return __tmp;
}

/* Tell modern GCC (12.x) to not complain that this infinite recursion is
 * bound to overflow the stack - this is exactly what this test wants to do :)
 *
 * Also, tell older versions of GCC that do not know about -Winfinit-recursion
 * that it is safe to ignore `GCC diagnostics ignored "-Winfinit-recursion"`.
 * They behave as intended in this case :)
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Winfinite-recursion"
static int recurse(int counter)
{
    if (buf.canary != CANARY_VALUE) {
        unsigned int sp = __get_PSP();
        printf("counter =%4d, SP = 0x%08x, canary = 0x%08x\n", counter, sp, buf.canary);
        printf("canary = 0x%08x\nTest failed.\n", buf.canary);

        for (;;) {
            thread_sleep();
        }
    }

    counter++;

    /* Recursing twice here prevents the compiler from optimizing-out the recursion. */
    return recurse(counter) + recurse(counter);
}
#pragma GCC diagnostic pop

static void *thread(void *arg)
{
    (void) arg;
    for (uint8_t i = 0; i < 8; i++)
    {
        print_pmpcfg(i);
    }
    thread_drop_privilege();
    recurse(0);

    return NULL;
}

int main(void)
{
    puts("\nPMP Stack Guard Test\n");
    for (size_t i = 0; i < 128; i++)
    {
        bugger[i] = 0xAA;
    }
    
    printf("bugger: %08X - %08X\n", (uintptr_t) bugger, (uintptr_t) &bugger[128-1]);
    puts("If the test fails, the canary value will change unexpectedly");
    puts("after ~150 iterations. If the test succeeds, the MEM MANAGE HANDLER");
    puts("will trigger a RIOT kernel panic before the canary value changes.\n");

#ifdef MODULE_PMP_STACK_GUARD
    puts("The pmp_stack_guard module is present. Expect the test to succeed.\n");
#else
    puts("The pmp_stack_guard module is missing! Expect the test to fail.\n");
#endif

    buf.canary = CANARY_VALUE;
    printf("Stack: %08X - %08X\n", (uintptr_t) buf.stack, (uintptr_t) &buf.stack[THREAD_STACKSIZE_MAIN-1]);
    printf("Canary: %08X\n", (uintptr_t) &buf.canary);
    thread_create(buf.stack, sizeof(buf.stack), THREAD_PRIORITY_MAIN - 1, 0, thread, NULL, "thread");

    return 0;
}
