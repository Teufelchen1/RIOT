/*
 * SPDX-FileCopyrightText: 2015 Kaspar Schleiser <kaspar@schleiser.de>
 * SPDX-FileCopyrightText: 2025 HAW Hamburg
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup drivers_slipmux
 * @{
 *
 * @file
 * @brief   Auto initialization for SLIPMUX devices
 *
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 * @author  Bennet Hattesen <bennet.hattesen@haw-hamburg.de>
 */

#include "auto_init_utils.h"
#include "auto_init_priorities.h"

extern void slipmux_init(void);

/**
 * @brief   Initialize all configured SLIPMUX devices
 */
void slipmux_dev_init_devs(void)
{
    slipmux_init();
}

AUTO_INIT(slipmux_dev_init_devs, AUTO_INIT_PRIO_MOD_SLIPMUX_DEV);

/** @} */
