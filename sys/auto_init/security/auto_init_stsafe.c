/*
 * Copyright (C) 2022 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_auto_init
 * @{
 * @file
 * @brief       Initializes StSafeA devices
 *
 * @author      Bennet Blischke <bennet.blischke@haw-hamburg.de>
 * @}
 */
#include "log.h"

#include "kernel_defines.h"

#define ENABLE_DEBUG 1
#include "debug.h"

#if IS_USED(MODULE_STSAFEA_CONTRIB)
#include "stsafea_types.h"
#include "stsafea_core.h"


//#if IS_ACTIVE(CONFIG_MODULE_PSA_CRYPTO)
#include "psa_crypto_se_management.h"

extern psa_drv_se_t stsafe_methods;
//#endif

static StSafeA_Handle_t stsafea_handle;
static uint8_t a_rx_tx_stsafea_data[STSAFEA_BUFFER_MAX_SIZE];

void auto_init_stsafe(void)
{
    DEBUG("StSafeA AUTO INIT\n");
    StSafeA_ResponseCode_t resp;

    if ((resp = StSafeA_Init(&stsafea_handle, a_rx_tx_stsafea_data)) != STSAFEA_OK) {
        LOG_ERROR("[auto_init_stsafe] error initializing StSafeA device");
    }
#if (IS_ACTIVE(CONFIG_MODULE_PSA_CRYPTO))
    psa_status_t status = psa_register_secure_element(PSA_KEY_LOCATION_SE_MIN + 1, &stsafe_methods, &stsafea_handle);
    if (status != PSA_SUCCESS) {
        LOG_ERROR(
            "[auto_init_stsafe] PSA Crypto – error registering StSafeA PSA driver \
            status: %li\n", status);
    }
#endif
}
#endif
