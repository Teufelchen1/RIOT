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
#include "native_internal.h"
#include "mprotect.h"

/* sysconf */
#include <unistd.h>
/* errno */
#include <errno.h>

#define ENABLE_DEBUG 1
#include "debug.h"


int mprotect(void *addr, size_t len, int prot) {
    int ret = real_mprotect(addr, len, prot);
    if (ret != 0) {
        switch (errno) {
            case EACCES:
                return MPROTECT_EACCES;
            case EINVAL:
                return MPROTECT_EINVAL;
            case ENOMEM:
                return MPROTECT_ENOMEM;
            default:
                return MPROTECT_ENOMEM;
        }
    }
    return MPROTECT_OK;
}

int mprotect_minimal(void *addr, int prot, size_t *range) {
    int pagesize = sysconf(_SC_PAGE_SIZE);
    *range = pagesize;
    return mprotect(addr, pagesize, prot);
}

/** @} */
