/*
 * Copyright (C) 2024 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#pragma once

/**
 * @defgroup    drivers_slipdev_base    SLIP device base
 * @ingroup     drivers_slipdev
 * @brief       Minimal SLIP device support for stdio and configuration
 *              without a full network stack.
 *
 * This module provides the common SLIP multiplexing infrastructure shared
 * between @ref drivers_slipdev_stdio and @ref drivers_slipdev_configuration,
 * allowing both to be used independently of the full @ref drivers_slipdev
 * network device.
 *
 * @{
 *
 * @file
 * @brief   SLIP device base definitions
 */

#include "slipdev.h"

#ifdef MODULE_SLIPDEV_CONFIG
#include "chunked_ringbuffer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Base device descriptor for slipdev without a network stack
 *
 * This struct contains only the state needed to multiplex stdio and/or
 * CoAP configuration frames over SLIP, without the network device fields.
 */
typedef struct {
    slipdev_params_t config;                /**< configuration parameters */
    slipdev_state_t state;                  /**< current device state */

#if IS_USED(MODULE_SLIPDEV_CONFIG)
    chunk_ringbuf_t rb_config;                      /**< Ringbuffer for configuration frames */
    uint8_t rxmem_config[CONFIG_SLIPDEV_BUFSIZE];   /**< memory used by configuration RX buffer */
    kernel_pid_t coap_server_pid;                   /**< PID of the CoAP server thread */
#endif
} slipdev_base_t;

/**
 * @brief   Setup and initialize a slipdev_base device
 *
 * This initializes the UART and sets up the SLIP multiplexer for stdio
 * and/or configuration use. When @ref drivers_slipdev (the network device)
 * is also in use, @ref slipdev_setup handles initialization instead.
 *
 * @param[in] dev       base device descriptor
 * @param[in] params    UART and baudrate configuration
 */
void slipdev_base_setup(slipdev_base_t *dev, const slipdev_params_t *params);

#ifdef __cplusplus
}
#endif

/** @} */
