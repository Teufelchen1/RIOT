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
#include <string.h>

#include "context_frame.h"
#include "thread.h"
#include "msg.h"

#define STACK_MARKER                (0x77777777)
#define STACK_SIZE 208
char second_thread_stack[STACK_SIZE];

/*
static inline uint32_t getSP(void) {
    uint32_t tmp = 0;
    __asm__ volatile ("mv %0, x2" : "=r" (tmp));
    return tmp;
}
*/
void print_stack(char *stack, size_t size) {
    thread_t *t = (thread_t *) (uintptr_t)(stack + size - sizeof(thread_t));

    printf("Stack from: 0x%08X to %08X (%d bytes)\n",
        (uintptr_t) stack, (uintptr_t) stack + size, size);
    printf("Thread control block at: 0x%08X\n", (uintptr_t) t);
    
    printf("Unused stack from 0x%08X to 0x%08X:\n",(uintptr_t) stack, (uintptr_t) t->sp);
    for(uint8_t * ptr = (uint8_t *) stack; (char *) ptr < t->sp; ptr += 8) {
        printf("0x%08X(8): %02X %02X %02X %02X %02X %02X %02X %02X\n", (uintptr_t) ptr,
        ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7]);
    }


    struct context_switch_frame *sf = (struct context_switch_frame *) (uintptr_t) t->sp;
    printf("Context switch frame from 0x%08X to 0x%08X\n", (uintptr_t) sf, (uintptr_t) sf + sizeof(struct context_switch_frame));
    printf("0x%08X(4) sf->s0:\t0x%08lX\n", (uintptr_t) &(sf->s0),sf->s0);
    printf("0x%08X(4) sf->s1:\t0x%08lX\n", (uintptr_t) &(sf->s1),sf->s1);
    printf("0x%08X(4) sf->s2:\t0x%08lX\n", (uintptr_t) &(sf->s2),sf->s2);
    printf("0x%08X(4) sf->s3:\t0x%08lX\n", (uintptr_t) &(sf->s3),sf->s3);
    printf("0x%08X(4) sf->s4:\t0x%08lX\n", (uintptr_t) &(sf->s4),sf->s4);
    printf("0x%08X(4) sf->s5:\t0x%08lX\n", (uintptr_t) &(sf->s5),sf->s5);
    printf("0x%08X(4) sf->s6:\t0x%08lX\n", (uintptr_t) &(sf->s6),sf->s6);
    printf("0x%08X(4) sf->s7:\t0x%08lX\n", (uintptr_t) &(sf->s7),sf->s7);
    printf("0x%08X(4) sf->s8:\t0x%08lX\n", (uintptr_t) &(sf->s8),sf->s8);
    printf("0x%08X(4) sf->s9:\t0x%08lX\n", (uintptr_t) &(sf->s9),sf->s9);
    printf("0x%08X(4) sf->s0:\t0x%08lX\n", (uintptr_t) &(sf->s0),sf->s0);
    printf("0x%08X(4) sf->s1:\t0x%08lX\n", (uintptr_t) &(sf->s1),sf->s1);
    printf("0x%08X(4) sf->ra:\t0x%08lX\n", (uintptr_t) &(sf->ra),sf->ra);
    printf("0x%08X(4) sf->t0:\t0x%08lX\n", (uintptr_t) &(sf->t0),sf->t0);
    printf("0x%08X(4) sf->t1:\t0x%08lX\n", (uintptr_t) &(sf->t1),sf->t1);
    printf("0x%08X(4) sf->t2:\t0x%08lX\n", (uintptr_t) &(sf->t2),sf->t2);
    printf("0x%08X(4) sf->t3:\t0x%08lX\n", (uintptr_t) &(sf->t3),sf->t3);
    printf("0x%08X(4) sf->t4:\t0x%08lX\n", (uintptr_t) &(sf->t4),sf->t4);
    printf("0x%08X(4) sf->t5:\t0x%08lX\n", (uintptr_t) &(sf->t5),sf->t5);
    printf("0x%08X(4) sf->t6:\t0x%08lX\n", (uintptr_t) &(sf->t6),sf->t6);
    printf("0x%08X(4) sf->a0:\t0x%08lX\n", (uintptr_t) &(sf->a0),sf->a0);
    printf("0x%08X(4) sf->a1:\t0x%08lX\n", (uintptr_t) &(sf->a1),sf->a1);
    printf("0x%08X(4) sf->a2:\t0x%08lX\n", (uintptr_t) &(sf->a2),sf->a2);
    printf("0x%08X(4) sf->a3:\t0x%08lX\n", (uintptr_t) &(sf->a3),sf->a3);
    printf("0x%08X(4) sf->a4:\t0x%08lX\n", (uintptr_t) &(sf->a4),sf->a4);
    printf("0x%08X(4) sf->a5:\t0x%08lX\n", (uintptr_t) &(sf->a5),sf->a5);
    printf("0x%08X(4) sf->a6:\t0x%08lX\n", (uintptr_t) &(sf->a6),sf->a6);
    printf("0x%08X(4) sf->a7:\t0x%08lX\n", (uintptr_t) &(sf->a7),sf->a7);
    printf("0x%08X(4) sf->pc:\t0x%08lX\n", (uintptr_t) &(sf->pc),sf->pc);
    printf("0x%08X(4) sf->pad[0]:\t0x%08lX\n", (uintptr_t) &(sf->pad[0]),sf->pad[0]);
    printf("0x%08X(4) sf->pad[1]:\t0x%08lX\n", (uintptr_t) &(sf->pad[1]),sf->pad[1]);
    printf("0x%08X(4) sf->pad[2]:\t0x%08lX\n", (uintptr_t) &(sf->pad[2]),sf->pad[2]);

    uint32_t *stackmarker = (uint32_t *) (sf + 1);
    while(*stackmarker != STACK_MARKER) {
        printf("0x%08X(4) padding:\t0x%08lX\n", (uintptr_t) stackmarker, *stackmarker);
        stackmarker++;
    }
    printf("0x%08X(4) stackmarker:\t0x%08lX\n", (uintptr_t) stackmarker, *stackmarker);


    printf("0x%08X(%d) t->sp:\t0x%08X (%d bytes available)\n",
        (uintptr_t) &(t->sp), sizeof(t->sp), (uintptr_t) t->sp, t->sp - stack);
    printf("0x%08X(%d) t->status:\t0x%08X\n",
        (uintptr_t) &(t->status), sizeof(t->status), (uintptr_t) t->status);
    printf("0x%08X(%d) t->priority:\t0x%08X\n",
        (uintptr_t) &(t->priority), sizeof(t->priority), (uintptr_t) t->priority);
    printf("0x%08X(%d) t->pid:\t0x%08X\n",
        (uintptr_t) &(t->pid), sizeof(t->pid), (uintptr_t) t->pid);
    printf("0x%08X(%d) t->rq_entry: ....\n",
        (uintptr_t) &(t->rq_entry), sizeof(t->rq_entry));
#if defined(DEVELHELP)
    printf("0x%08X(%d) t->stack_start:\t0x%08X\n",
        (uintptr_t) &(t->stack_start), sizeof(t->stack_start), (uintptr_t) t->stack_start);
#endif
#if defined(CONFIG_THREAD_NAMES)
    printf("0x%08X(%d) t->name:\t%s\n",
        (uintptr_t) &(t->name), sizeof(t->name), t->name);
#endif
#if defined(DEVELHELP)
    printf("0x%08X(%d) t->stack_size:\t0x%08X\n",
        (uintptr_t) &(t->stack_size), sizeof(t->stack_size), (unsigned int) t->stack_size);
#endif
}


void *second_thread(void *arg)
{
    (void) arg;
    uint8_t mem;
    memcpy(mem, buff, )
    return NULL;
}

int main(void)
{
    printf("Starting IPC Ping-pong example...\n");
    printf("1st thread started, pid: %" PRIkernel_pid "\n", thread_getpid());

    msg_t m;
    kernel_pid_t pid = thread_create(second_thread_stack, sizeof(second_thread_stack),
                            THREAD_PRIORITY_MAIN, 0,
                            second_thread, NULL, "pong");

    print_stack(second_thread_stack, STACK_SIZE);
    while (1) {
        msg_send_receive(&m, &m, pid);
        //printf("1st: Got msg with content %u\n", (unsigned int)m.content.value);
    }
}
