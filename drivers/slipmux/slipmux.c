/*
 * Copyright (C) 2017 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  Martine Lenders <mlenders@inf.fu-berlin.de>
 * @author  Benjamin Valentin <benjamin.valentin@ml-pa.com>
 * @author  Bennet Hattesen <bennet.hattesen@haw-hamburg.de>
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "log.h"
//#include "slipdev.h"
//#include "SLIPMUX_internal.h"
#include "net/eui_provider.h"

/* XXX: BE CAREFUL ABOUT USING OUTPUT WITH MODULE_SLIPMUX_STDIO IN SENDING
 * FUNCTIONALITY! MIGHT CAUSE DEADLOCK!!!1!! */
#define ENABLE_DEBUG 1
#include "debug.h"

#include "slipmux_internal.h"
#include "slipmux_params.h"
#include "auto_init_utils.h"
#include "isrpipe.h"
#include "mutex.h"
#if IS_USED(MODULE_SLIPMUX_COAP)
#include "checksum/crc16_ccitt.h"
#include "net/nanocoap.h"
#endif
#include "stdio_uart.h"

#if (IS_USED(MODULE_SLIPMUX_STDIO) || IS_USED(MODULE_SLIPMUX_COAP))
/* For synchronization with stdio/config threads */
mutex_t slipmux_mutex = MUTEX_INIT;
#endif

#include "periph/uart.h"
#include "chunked_ringbuffer.h"


static slipmux_t slipmuxdev;

void _slipmux_rx_cb(void *arg, uint8_t byte)
{
    slipmux_t *dev = arg;

    switch (dev->state) {
    case SLIPMUX_STATE_STDIN:
        switch (byte) {
        case SLIPMUX_ESC:
            dev->state = SLIPMUX_STATE_STDIN_ESC;
            break;
        case SLIPMUX_END:
            dev->state = SLIPMUX_STATE_NONE;
            break;
        default:
#if IS_USED(MODULE_SLIPMUX_STDIO)
            if (dev->config.uart == STDIO_UART_DEV) {
                isrpipe_write_one(&stdin_isrpipe, byte);
            }
#endif
            break;
        }
        return;
    case SLIPMUX_STATE_STDIN_ESC:
        switch (byte) {
        case SLIPMUX_END_ESC:
            byte = SLIPMUX_END;
            break;
        case SLIPMUX_ESC_ESC:
            byte = SLIPMUX_ESC;
            break;
        }
        dev->state = SLIPMUX_STATE_STDIN;
#if IS_USED(MODULE_SLIPMUX_STDIO)
        if (dev->config.uart == STDIO_UART_DEV) {
            isrpipe_write_one(&stdin_isrpipe, byte);
        }
#endif
        return;
    case SLIPMUX_STATE_COAP:
        switch (byte) {
        case SLIPMUX_ESC:
            dev->state = SLIPMUX_STATE_COAP_ESC;
            break;
        case SLIPMUX_END:
            dev->state = SLIPMUX_STATE_NONE;
#if IS_USED(MODULE_SLIPMUX_COAP)
            crb_end_chunk(&dev->coap_rb, true);
            thread_flags_set(thread_get(dev->coap_server_pid), 1);
#endif
            break;
        default:
#if IS_USED(MODULE_SLIPMUX_COAP)
            /* discard frame if byte can't be added */
            if (!crb_add_byte(&dev->coap_rb, byte)) {
                DEBUG("slipmux: rx buffer full, drop frame\n");
                crb_end_chunk(&dev->coap_rb, false);
                dev->state = SLIPMUX_STATE_NONE;
                return;
            }
#endif
            break;
        }
        return;
    case SLIPMUX_STATE_COAP_ESC:
        switch (byte) {
        case SLIPMUX_END_ESC:
            byte = SLIPMUX_END;
            break;
        case SLIPMUX_ESC_ESC:
            byte = SLIPMUX_ESC;
            break;
        }
#if IS_USED(MODULE_SLIPMUX_COAP)
        /* discard frame if byte can't be added */
        if (!crb_add_byte(&dev->coap_rb, byte)) {
            DEBUG("slipmux: rx buffer full, drop frame\n");
            crb_end_chunk(&dev->coap_rb, false);
            dev->state = SLIPMUX_STATE_NONE;
            return;
        }
#endif
        dev->state = SLIPMUX_STATE_COAP;
        return;
    case SLIPMUX_STATE_NET:
        switch (byte) {
        case SLIPMUX_ESC:
            dev->state = SLIPMUX_STATE_NET_ESC;
            return;
        case SLIPMUX_END:
#if IS_USED(MODULE_SLIPMUX_NET)
            crb_end_chunk(&dev->net_rb, true);
            netdev_trigger_event_isr(&dev->netdev);
#endif
            dev->state = SLIPMUX_STATE_NONE;
            return;
        }
        /* discard frame if byte can't be added */
        if (!crb_add_byte(&dev->net_rb, byte)) {
            DEBUG("slipdev: rx buffer full, drop frame\n");
            crb_end_chunk(&dev->net_rb, false);
            dev->state = SLIPMUX_STATE_NONE;
        }
        return;
    /* escaped byte received */
    case SLIPMUX_STATE_NET_ESC:
        switch (byte) {
        case SLIPMUX_END_ESC:
            byte = SLIPMUX_END;
            break;
        case SLIPMUX_ESC_ESC:
            byte = SLIPMUX_ESC;
            break;
        }
#if IS_USED(MODULE_SLIPMUX_NET)
        /* discard frame if byte can't be added */
        if (!crb_add_byte(&dev->net_rb, byte)) {
            DEBUG("slipdev: rx buffer full, drop frame\n");
            crb_end_chunk(&dev->net_rb, false);
            dev->state = SLIPMUX_STATE_NONE;
            return;
        }
#endif
        dev->state = SLIPMUX_STATE_NET;
        return;

    case SLIPMUX_STATE_NONE:
        /* is diagnostic frame? */
        if (byte == SLIPMUX_STDIO_START) {
            dev->state = SLIPMUX_STATE_STDIN;
            return;
        }

        if (byte == SLIPMUX_COAP_START) {
#if IS_USED(MODULE_SLIPMUX_COAP)
            /* try to create new configuration / CoAP frame */
            if (!crb_start_chunk(&dev->coap_rb)) {
                return;
            }
#endif
            dev->state = SLIPMUX_STATE_COAP;
            return;
        }

        if (
            /* is it IPv4 packet? */
            (byte >= 0x45 && byte <= 0x4f) ||
            /* or is it IPv6 packet? */
            (byte >= 0x60 && byte <= 0x6f)
        ) {
#if IS_USED(MODULE_SLIPMUX_NET)
            /* try to create new ip frame */
            if (!crb_start_chunk(&dev->net_rb)) {
                DEBUG("slipmux: can't start new net frame, drop frame\n");
                return;
            }
            if (!crb_add_byte(&dev->net_rb, byte)) {
                DEBUG("slipdev: rx buffer full, drop frame\n");
                crb_end_chunk(&dev->net_rb, false);
                dev->state = SLIPMUX_STATE_NONE;
                return;
            }
#endif
            dev->state = SLIPMUX_STATE_NET;
            return;
        }

        /* ignore empty frame */
        if (byte == SLIPMUX_END) {
            return;
        }

        DEBUG("slipmux: Unkown start byte %b ignored\n", byte);
        // todo: Own parser category for unknown packet
    }
}


void slipmux_write_bytes(uart_t uart, const uint8_t *data, size_t len)
{
    for (unsigned j = 0; j < len; j++, data++) {
        switch (*data) {
            case SLIPMUX_END:
                /* escaping END byte*/
                slipmux_write_byte(uart, SLIPMUX_ESC);
                slipmux_write_byte(uart, SLIPMUX_END_ESC);
                break;
            case SLIPMUX_ESC:
                /* escaping ESC byte*/
                slipmux_write_byte(uart, SLIPMUX_ESC);
                slipmux_write_byte(uart, SLIPMUX_ESC_ESC);
                break;
            default:
                slipmux_write_byte(uart, *data);
        }
    }
}

void slipmux_init(void)
{
    slipmuxdev.config = slipmux_params[0];

    DEBUG("slipmux: initializing device %p on UART %i with baudrate %" PRIu32 "\n",
          (void *)&slipmuxdev, slipmuxdev.config.uart, slipmuxdev.config.baudrate);
    if (uart_init(slipmuxdev.config.uart, slipmuxdev.config.baudrate, _slipmux_rx_cb,
                  &slipmuxdev) != UART_OK) {
        LOG_ERROR("slipmux: error initializing UART %i with baudrate %" PRIu32 "\n",
                  slipmuxdev.config.uart, slipmuxdev.config.baudrate);
        return;
    }
#if IS_USED(MODULE_SLIPMUX_COAP)
    extern void slipmux_coap_init(slipmux_t *dev);
    slipmux_coap_init(&slipmuxdev);
#endif
#if IS_USED(MODULE_SLIPMUX_NET)
    extern void slipmux_net_init(slipmux_t *dev);
    slipmux_net_init(&slipmuxdev);
#endif
}

AUTO_INIT(slipmux_init, 9999);
/** @} */
