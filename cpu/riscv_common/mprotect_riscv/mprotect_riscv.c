/*
 * Copyright (C) 2019 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  Martine S. Lenders <m.lenders@fu-berlin.de>
 */
#include "kernel_defines.h"
#include "mprotect.h"
#include "pmp.h"

#define ENABLE_DEBUG 1
#include "debug.h"


int mprotect(void *addr, size_t len, int prot) {
    (void) addr;
    (void) len;
    (void) prot;
    DEBUG("Hello from mprotect riscv\n");
    if (len == 4){
        pmp_configure(0, (uintptr_t) addr, PMP_L | PMP_NA4 | PMP_R);
        print_pmp_reg(0);
        return MPROTECT_OK;
    } else {
        uint32_t flags = 0;
        while(len > ((unsigned) 8 << flags)) {
            DEBUG("len(%d) > %d\n", len, (8 << flags));
            flags++;
        }
        pmp_configure(0, (uintptr_t) addr, PMP_NAPOT | PMP_R);
        print_pmp_reg(0);
        return MPROTECT_OK;
    }
    return MPROTECT_NOTIMPLEMENTED;
}

int mprotect_minimal(void *addr, int prot, size_t *range) {
    *range = 4;
    return mprotect(addr, 4, prot);
}

/** @} */
