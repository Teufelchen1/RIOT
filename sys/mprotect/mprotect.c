/*
 * Copyright (C) 2023 Bennet Blischke <bennet.blischke@haw-hamburg.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
#include "mprotect.h"

#define ENABLE_DEBUG 0
#include "debug.h"

void print_mprotect_status(enum MPROTECT_STATUS status) {
    switch (status) {
        case MPROTECT_OK:
            puts("Success");
            break;
        case MPROTECT_EACCES:
            puts("Access error");
            break;
        case MPROTECT_EINVAL:
            puts("Invalid address");
            break;
        case MPROTECT_ENOMEM:
            puts("Internal error");
            break;
        case MPROTECT_NOTIMPLEMENTED:
            puts("Not implemented");
            break;
        default:
            puts("?????");
            break;
    };
}

int __attribute__((weak)) mprotect(void *addr, size_t len, int prot) {
    (void) addr;
    (void) len;
    (void) prot;
    return MPROTECT_NOTIMPLEMENTED;
}

int __attribute__((weak)) mprotect_minimal(void *addr, int prot, size_t *range) {
    (void) addr;
    (void) prot;
    (void) range;
    return MPROTECT_NOTIMPLEMENTED;
}

