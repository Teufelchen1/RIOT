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
 * @brief Test application for the mpu_stack_guard pseudo-module
 *
 * @author Ian Martin <ian@locicontrols.com>
 *
 * @}
 */

#include <stdio.h>

#include "cpu.h"
#include "thread.h"
#include "pmp.h"

/* RIOT's PMP headers gracefully fail when no PMP is present.
 * Use this to catch if RIOT's features are correctly gating PMP use.
 */
#if !__PMP_PRESENT
#error "(!__PMP_PRESENT)"
#endif

#define CANARY_VALUE 0xdeadbeef

static struct {
    unsigned int canary;
    char stack[THREAD_STACKSIZE_MAIN];
} buf;

unsigned int __get_PSP(void) {
    uint32_t sp = 0;
    __asm__ __volatile__ ("add %0, x0, x2": "+r" (sp));
    return sp;
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
    int i = 1;
    printf("i_addr= %p, counter =%4d, SP = 0x%08x, canary = 0x%08x\n", &i, counter, (unsigned int)__get_PSP(), buf.canary);

    if (buf.canary != CANARY_VALUE) {
        printf("canary = 0x%08x\nTest failed.\n", buf.canary);
        i++;
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
    pmp_configure(0, 0x80003e80, PMP_L|PMP_TOR | PMP_R);
    unsigned int * mem = (unsigned int *)0x80000200;
    for(int i = 0; i < 6; i++)
        print_pmp_reg(i);
    printf("mem_addr = %p, mem_val = %x, counter =%4d, SP = 0x%08x, canary = 0x%08x\n", mem, *mem, -1, (unsigned int)__get_PSP(), buf.canary);
    *mem = 0xFFFFFFFF;
    printf("mem_addr = %p, mem_val = %x, counter =%4d, SP = 0x%08x, canary = 0x%08x\n", mem, *mem, -1, (unsigned int)__get_PSP(), buf.canary);
    recurse(0);

    return NULL;
}

int main(void)
{
    puts("\nPMP Stack Guard Test\n");

    puts("If the test fails, the canary value will change unexpectedly");
    puts("after ~150 iterations. If the test succeeds, the MEM MANAGE HANDLER");
    puts("will trigger a RIOT kernel panic before the canary value changes.\n");

    //for(int i = 0; i < 6; i++)
    //    print_pmp_reg(i);

#ifdef MODULE_PMP_STACK_GUARD
    puts("The mpu_stack_guard module is present. Expect the test to succeed.\n");
#else
    puts("The mpu_stack_guard module is missing! Expect the test to fail.\n");
#endif

    buf.canary = CANARY_VALUE;
    printf("Buf canary: %p\n", &buf.canary);
    printf("Buf start: %p, to %p\n", buf.stack, buf.stack + sizeof(buf.stack));
    thread_create(buf.stack, sizeof(buf.stack), THREAD_PRIORITY_MAIN - 1, 0, thread, NULL, "thread");

    return 0;
}
