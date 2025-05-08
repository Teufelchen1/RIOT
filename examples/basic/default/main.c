/*
 * Copyright (C) 2008, 2009, 2010 Kaspar Schleiser <kaspar@schleiser.de>
 * Copyright (C) 2013 INRIA
 * Copyright (C) 2013 Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Default application that shows a lot of functionality of RIOT
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "shell.h"

#include "net/gnrc/pktdump.h"
#include "net/gnrc.h"

#include "nanocbor/nanocbor.h"

ssize_t _sample_command_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                  coap_request_ctx_t *context)
{
    (void) context;
    char buffer[100];

    nanocbor_value_t decoder;
    nanocbor_value_t array;
    nanocbor_decoder_init(&decoder, pkt->payload, pkt->payload_len);
    nanocbor_enter_array(&decoder, &array);
    bool caps = false;
    uint8_t repeats = 1;
    if ((nanocbor_get_bool(&array, &caps) < 0) || (nanocbor_get_uint8(&array, &repeats) < 0)) {
      return coap_reply_simple(pkt, COAP_CODE_UNPROCESSABLE_ENTITY, buf, len,
            COAP_FORMAT_NONE, NULL, 0);
    }

    for (int i = 0; i < 100; ++i)
    {
        buffer[i] = 'a';
    }
    buffer[99] = 0;

    char * desc = "Hello ";
    char * descCap = "HELLO ";

    for (int i = 0; i < repeats; ++i)
    {
        if (caps) {
            memcpy(&buffer[strlen(descCap)*i], descCap, strlen(descCap)+1);
        } else {
            memcpy(&buffer[strlen(desc)*i], desc, strlen(desc)+1);
        }
    }
    // pkt->payload, pkt->payload_len
    

     return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
            COAP_FORMAT_CBOR, (uint8_t*)buffer, strlen(buffer));
}

NANOCOAP_RESOURCE(sample_cmd) { \
    .path = "/SampleCommand", .methods = COAP_POST, .handler = _sample_command_handler, .context = NULL \
};

ssize_t _mem_read_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                  coap_request_ctx_t *context)
{
    (void) context;
    uint8_t buffer[260];

    nanocbor_value_t decoder;
    nanocbor_value_t array;
    nanocbor_decoder_init(&decoder, pkt->payload, pkt->payload_len);
    nanocbor_enter_array(&decoder, &array);

    uint32_t addr = 0;
    uint8_t size = 0;
    if ((nanocbor_get_uint32(&array, &addr) < 0) || (nanocbor_get_uint8(&array, &size) < 0)) {
      return coap_reply_simple(pkt, COAP_CODE_UNPROCESSABLE_ENTITY, buf, len,
            COAP_FORMAT_NONE, NULL, 0);
    }

    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buffer, sizeof(buffer));
    nanocbor_put_bstr(&enc,(uint8_t *) addr, size);
    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
            COAP_FORMAT_CBOR, (uint8_t*)buffer, nanocbor_encoded_len(&enc));
}

NANOCOAP_RESOURCE(mem_cmd) { \
    .path = "/Memory", .methods = COAP_POST, .handler = _mem_read_handler, .context = NULL \
};


int main(void)
{
#ifdef MODULE_GNRC_PKTDUMP
    gnrc_netreg_entry_t dump = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                          gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_UNDEF, &dump);
#endif

    (void) puts("Welcome to RIOT!");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
