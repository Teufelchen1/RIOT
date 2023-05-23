/*
 * Copyright (C) 2018 Tobias Heider <heidert@nm.ifi.lmu.de>
 *               2020 Koen Zandberg <koen@bergzand.net>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
/**
 * @defgroup    sys_memarray memory array allocator
 * @ingroup     sys_memory_management
 * @brief       memory array allocator
 * @{
 *
 * @brief       pseudo dynamic allocation in static memory arrays
 * @author      Tobias Heider <heidert@nm.ifi.lmu.de>
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef MPROTECT_H
#define MPROTECT_H

/* Include for puts() */
//#include <stdio.h>
/* Include size_t definition */
#include <stdlib.h>

#ifdef BOARD_NATIVE
/* PROT_* */
#include <sys/mman.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BOARD_NATIVE
#define MPROTECT_NONE PROT_NONE
#define MPROTECT_READ PROT_READ
#define MPROTECT_WRITE PROT_WRITE
#define MPROTECT_EXEC PROT_EXEC
#else
#define MPROTECT_NONE 0
#define MPROTECT_READ 1
#define MPROTECT_WRITE 2
#define MPROTECT_EXEC 4
#endif

enum MPROTECT_STATUS {
    /* Success */
    MPROTECT_OK,
    /* Access error, e.g. insufficient privileges */
    MPROTECT_EACCES,
    /* Invalid memory address, e.g. alignemt doesn't match the requierments */
    MPROTECT_EINVAL,
    /* Internal error within mprotect or RIOT */
    MPROTECT_ENOMEM,
    MPROTECT_NOTIMPLEMENTED,
};

void print_mprotect_status(enum MPROTECT_STATUS status);


/**
 * @brief Set the access protection of given memory space
 *
 * @param[in]     addr   lower address of target memory space
 * @param[in]     len    length of the memory space
 * @param[in]     prot   access mode to be set
 */
int mprotect(void *addr, size_t len, int prot);

/**
 * @brief Set the access protection for the smallest possible memory space
 *        for a given start address
 *
 * @param[in]     addr   lower address of target memory space
 * @param[in]     prot   access mode to be set
 * @param[out]    range  on success, length of the memory space, zero otherwise
 */
int mprotect_minimal(void *addr, int prot, size_t *range);

#ifdef __cplusplus
}
#endif

#endif /* MPROTECT_H */

/**
 * @}
 */
