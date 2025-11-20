/*
 * Copyright (C) 2018 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#pragma once

/**
 * @ingroup drivers_slipdev
 * @{
 *
 * @file
 * @internal
 * @brief   Internal SLIP device definitions
 *
 * @author  Martine Lenders <m.lenders@fu-berlin.de>
 */

#include <stddef.h>
#include <stdint.h>

#include "isrpipe.h"
#include "periph/uart.h"
#include "mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   ISR pipe to hand read bytes to stdin
 */
extern isrpipe_t slipmux_stdio_isrpipe;

/**
 * @brief   Mutex to synchronize write operations to the UART between stdio
 *          sub-module and normal SLIP.
 */
extern mutex_t slipmux_mutex;

/**
 * @name    SLIP(MUX) marker bytes
 * @see     [RFC 1055](https://tools.ietf.org/html/rfc1055)
 * @{
 */
#define SLIPMUX_END         (0xc0U)
#define SLIPMUX_ESC         (0xdbU)
#define SLIPMUX_END_ESC     (0xdcU)
#define SLIPMUX_ESC_ESC     (0xddU)

/**
 * @brief   Marker byte for beginning of stdio
 * @see     taken from diagnostic transfer from
 *          [SLIPMUX](https://tools.ietf.org/html/draft-bormann-t2trg-slipmux-03#section-4)
 */
#define SLIPMUX_STDIO_START (0x0aU)

/**
 * @brief   Marker byte for beginning of configuration/CoAP
 * @see     taken from configuration from
 *          [SLIPMUX](https://tools.ietf.org/html/draft-bormann-t2trg-slipmux-03#section-5)
 */
#define SLIPMUX_COAP_START (0xa9U)

enum {
    /* Device is in no mode (currently did not receiving any data frame) */
    SLIPMUX_STATE_NONE = 0,
    /* Device writes handles data as network device */
    SLIPMUX_STATE_NET,
    /* Device writes handles data as network device, next byte is escaped */
    SLIPMUX_STATE_NET_ESC,
    /* Device writes received data to stdin */
    SLIPMUX_STATE_STDIN,
    /* Device writes received data to stdin, next byte is escaped */
    SLIPMUX_STATE_STDIN_ESC,
    /* Device writes received data as CoAP message */
    SLIPMUX_STATE_COAP,
    /* Device writes received data as CoAP message, next byte is escaped */
    SLIPMUX_STATE_COAP_ESC,
    /* Device is in standby, will wake up when sending data */
    SLIPMUX_STATE_STANDBY,
    /* Device is in sleep mode */
    SLIPMUX_STATE_SLEEP,
};

void _slipmux_rx_cb(void *arg, uint8_t byte);

/**
 * @brief   Writes one byte to UART
 *
 * @param[in] uart  The UART device to write to.
 * @param[in] byte  The byte to write to @p uart.
 */
static inline void slipmux_write_byte(uart_t uart, uint8_t byte)
{
    uart_write(uart, &byte, 1U);
}

/**
 * @brief   Write multiple bytes SLIPMUX-escaped to UART
 *
 * @param[in] uart  The UART device to write to.
 * @param[in] data  The bytes to write SLIPMUX-escaped to @p uart.
 * @param[in] len   Number of bytes in @p data.
 */
void slipmux_write_bytes(uart_t uart, const uint8_t *data, size_t len);

static inline void slipmux_lock(void)
{
    if (IS_USED(MODULE_SLIPMUX_STDIO) || IS_USED(MODULE_SLIPMUX_COAP)) {
       mutex_lock(&slipmux_mutex);
    }
}

static inline void slipmux_unlock(void)
{
    if (IS_USED(MODULE_SLIPMUX_STDIO) || IS_USED(MODULE_SLIPMUX_COAP)) {
       mutex_unlock(&slipmux_mutex);
    }
}

#ifdef __cplusplus
}
#endif

/** @} */