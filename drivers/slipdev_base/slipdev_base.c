/*
 * Copyright (C) 2024 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @brief   SLIP device base implementation
 *
 * Provides the common SLIP multiplexing infrastructure (mutex, write
 * primitives, RX state machine) for use by slipdev_stdio and
 * slipdev_config without requiring the full network device stack.
 */

#include <stdbool.h>
#include <stdint.h>

#include "mutex.h"
#include "slipdev_base.h"
#include "slipdev_internal.h"
#include "slipdev_params.h"
#include "kernel_defines.h"

#ifdef MODULE_SLIPDEV_STDIO
#include "isrpipe.h"
#include "stdio_base.h"
#endif

#define ENABLE_DEBUG 0
#include "debug.h"

#if IS_USED(MODULE_SLIPDEV_CONFIG)
#include "checksum/crc16_ccitt.h"
#include "net/nanocoap.h"
#endif

/* For synchronization with stdio/config threads */
mutex_t slipdev_mutex = MUTEX_INIT;

void slipdev_write_bytes(uart_t uart, const uint8_t *data, size_t len)
{
    for (unsigned j = 0; j < len; j++, data++) {
        switch (*data) {
        case SLIPDEV_END:
            /* escaping END byte */
            slipdev_write_byte(uart, SLIPDEV_ESC);
            slipdev_write_byte(uart, SLIPDEV_END_ESC);
            break;
        case SLIPDEV_ESC:
            /* escaping ESC byte */
            slipdev_write_byte(uart, SLIPDEV_ESC);
            slipdev_write_byte(uart, SLIPDEV_ESC_ESC);
            break;
        default:
            slipdev_write_byte(uart, *data);
        }
    }
}

#if IS_USED(MODULE_SLIPDEV_CONFIG)
/* The special init is the result of normal fcs init combined with slipmux config start (0xa9) */
#define SPECIAL_INIT_FCS (0x374cU)
#define COAP_STACKSIZE (1024)

static char _coap_stack[COAP_STACKSIZE];

static inline bool _config_start_frame(slipdev_base_t *dev)
{
    return crb_start_chunk(&dev->rb_config);
}

static inline void _config_end_frame(slipdev_base_t *dev)
{
    crb_end_chunk(&dev->rb_config, true);
    thread_flags_set(thread_get(dev->coap_server_pid), 1);
}

static inline bool _config_add_to_frame(slipdev_base_t *dev, uint8_t byte)
{
    if (!crb_add_byte(&dev->rb_config, byte)) {
        DEBUG("slipmux: coap rx buffer full, drop frame\n");
        crb_end_chunk(&dev->rb_config, false);
        return false;
    }
    return true;
}

static void *_coap_server_thread(void *arg)
{
    static uint8_t buf[512];
    slipdev_base_t *dev = arg;

    while (1) {
        thread_flags_wait_any(1);
        size_t len;
        while (crb_get_chunk_size(&dev->rb_config, &len)) {
            if (len > sizeof(buf)) {
                crb_consume_chunk(&dev->rb_config, NULL, len);
                continue;
            }
            crb_consume_chunk(&dev->rb_config, buf, len);

            /* Is the crc correct via residue(=0xF0B8) test */
            if (crc16_ccitt_fcs_update(SPECIAL_INIT_FCS, buf, len) != 0xF0B8) {
                break;
            }

            /* cut off the FCS checksum at the end */
            size_t pktlen = len - 2;

            coap_pkt_t pkt;
            sock_udp_ep_t remote;
            coap_request_ctx_t ctx = {
                .remote = &remote,
            };
            if (coap_parse(&pkt, buf, pktlen) < 0) {
                break;
            }
            unsigned int res = 0;
            if ((res = coap_handle_req(&pkt, buf, sizeof(buf), &ctx)) <= 0) {
                break;
            }

            uint16_t fcs_sum = crc16_ccitt_fcs_finish(SPECIAL_INIT_FCS, buf, res);

            mutex_lock(&slipdev_mutex);
            slipdev_write_byte(dev->config.uart, SLIPDEV_START_COAP);
            slipdev_write_bytes(dev->config.uart, buf, res);
            slipdev_write_bytes(dev->config.uart, (uint8_t *)&fcs_sum, 2);
            slipdev_write_byte(dev->config.uart, SLIPDEV_END);
            mutex_unlock(&slipdev_mutex);
        }
    }

    return NULL;
}
#endif /* IS_USED(MODULE_SLIPDEV_CONFIG) */

static inline void _stdio_add_to_frame(slipdev_base_t *dev, uint8_t byte)
{
#ifdef MODULE_SLIPDEV_STDIO
    if (dev->config.uart != slipdev_params[0].uart) {
        return;
    }
    isrpipe_write_one(&stdin_isrpipe, byte);
#else
    (void)dev;
    (void)byte;
#endif
}

static void _slip_rx_cb(void *arg, uint8_t byte)
{
    slipdev_base_t *dev = arg;

    switch (dev->state) {
    case SLIPDEV_STATE_STANDBY:
    /* fall through */
    case SLIPDEV_STATE_SLEEP:
        /* do nothing */
        break;
    case SLIPDEV_STATE_STDIN:
        switch (byte) {
        case SLIPDEV_ESC:
            dev->state = SLIPDEV_STATE_STDIN_ESC;
            break;
        case SLIPDEV_END:
            dev->state = SLIPDEV_STATE_NONE;
            break;
        default:
            _stdio_add_to_frame(dev, byte);
            break;
        }
        return;
    case SLIPDEV_STATE_STDIN_ESC:
        switch (byte) {
        case SLIPDEV_END_ESC:
            byte = SLIPDEV_END;
            break;
        case SLIPDEV_ESC_ESC:
            byte = SLIPDEV_ESC;
            break;
        }
        dev->state = SLIPDEV_STATE_STDIN;
        _stdio_add_to_frame(dev, byte);
        return;
    case SLIPDEV_STATE_CONFIG:
#if IS_USED(MODULE_SLIPDEV_CONFIG)
        switch (byte) {
        case SLIPDEV_ESC:
            dev->state = SLIPDEV_STATE_CONFIG_ESC;
            break;
        case SLIPDEV_END:
            dev->state = SLIPDEV_STATE_NONE;
            _config_end_frame(dev);
            break;
        default:
            if (!_config_add_to_frame(dev, byte)) {
                dev->state = SLIPDEV_STATE_UNKNOWN;
            }
            break;
        }
#else
        if (byte == SLIPDEV_END) {
            dev->state = SLIPDEV_STATE_NONE;
        }
        else {
            dev->state = SLIPDEV_STATE_UNKNOWN;
        }
#endif
        return;
    case SLIPDEV_STATE_CONFIG_ESC:
#if IS_USED(MODULE_SLIPDEV_CONFIG)
        switch (byte) {
        case SLIPDEV_END_ESC:
            byte = SLIPDEV_END;
            break;
        case SLIPDEV_ESC_ESC:
            byte = SLIPDEV_ESC;
            break;
        }
        if (_config_add_to_frame(dev, byte)) {
            dev->state = SLIPDEV_STATE_CONFIG;
        }
        else {
            dev->state = SLIPDEV_STATE_UNKNOWN;
        }
#else
        dev->state = SLIPDEV_STATE_UNKNOWN;
#endif
        return;
    case SLIPDEV_STATE_NET:
    case SLIPDEV_STATE_NET_ESC:
        /* NET frames are not processed without the network device; drop them */
        if (byte == SLIPDEV_END) {
            dev->state = SLIPDEV_STATE_NONE;
        }
        return;
    case SLIPDEV_STATE_UNKNOWN:
        if (byte == SLIPDEV_END) {
            dev->state = SLIPDEV_STATE_NONE;
        }
        return;
    case SLIPDEV_STATE_NONE:
        /* is diagnostic frame? */
        if (byte == SLIPDEV_START_STDIO) {
            dev->state = SLIPDEV_STATE_STDIN;
            return;
        }

#if IS_USED(MODULE_SLIPDEV_CONFIG)
        if (byte == SLIPDEV_START_COAP) {
            if (_config_start_frame(dev)) {
                dev->state = SLIPDEV_STATE_CONFIG;
            }
            else {
                dev->state = SLIPDEV_STATE_UNKNOWN;
            }
            return;
        }
#endif

        /* drop net frames and other unknown frames */
        if (byte != SLIPDEV_END) {
            dev->state = SLIPDEV_STATE_UNKNOWN;
            DEBUG("slipmux: Unknown or net start byte %02x, dropped\n", byte);
        }
        return;
    }
}

void slipdev_base_setup(slipdev_base_t *dev, const slipdev_params_t *params)
{
    dev->config = *params;
    dev->state = SLIPDEV_STATE_NONE;

#if IS_USED(MODULE_SLIPDEV_CONFIG)
    crb_init(&dev->rb_config, dev->rxmem_config, sizeof(dev->rxmem_config));

    dev->coap_server_pid = thread_create(_coap_stack, sizeof(_coap_stack),
                                         THREAD_PRIORITY_MAIN - 1,
                                         THREAD_CREATE_STACKTEST,
                                         _coap_server_thread,
                                         (void *)dev, "Slipmux CoAP server");
#endif

    uart_init(dev->config.uart, dev->config.baudrate, _slip_rx_cb, dev);
    slipdev_write_byte(dev->config.uart, SLIPDEV_END);
}

/** @} */
