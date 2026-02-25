/*
 * Copyright (C) 2024 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

/**
 * @ingroup sys_auto_init
 * @{
 *
 * @file
 * @brief   Auto initialization for slipdev_base (SLIP stdio/config without
 *          the network device stack)
 *
 * When the full @ref drivers_slipdev network device is used, its auto
 * initialization (in the network auto_init) handles the UART setup.
 * This module only runs when @ref drivers_slipdev is NOT used, providing
 * standalone initialization for @ref drivers_slipdev_stdio and/or
 * @ref drivers_slipdev_configuration.
 */

#include "log.h"

#include "slipdev_base.h"
#include "slipdev_params.h"

#define SLIPDEV_NUM ARRAY_SIZE(slipdev_params)

void auto_init_slipdev_base(void)
{
#ifndef MODULE_SLIPDEV
    /* When the full slipdev network driver is used, auto_init_slipdev()
     * in the network auto_init handles initialization. Nothing to do here. */
    static slipdev_base_t _devs[SLIPDEV_NUM];
    for (unsigned i = 0; i < SLIPDEV_NUM; i++) {
        LOG_DEBUG("[auto_init] initializing slipdev_base #%u\n", i);
        slipdev_base_setup(&_devs[i], &slipdev_params[i]);
    }
#endif
}

/** @} */
