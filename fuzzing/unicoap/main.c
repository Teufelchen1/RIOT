/*
 * Copyright (C) 2019 Sören Tempel <tempel@uni-bremen.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <err.h>
#include <stdlib.h>

#include "debug.h"
#include "net/unicoap/transport.h"
#include "private.h"
#include "architecture.h"
#include "private/packet.h" // unicoap_packet_t
#include "private/messaging.h" // gets UNICOAP_MESSAGING_RFC7252_EVENT_RX

__AFL_FUZZ_INIT()

#pragma clang optimize off

int main(void)
{
    __AFL_INIT();
    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) {
        int len = __AFL_FUZZ_TESTCASE_LEN;

        unicoap_endpoint_t remote = {
            .proto = UNICOAP_PROTO_UDP,
            .slipmux_ep = NULL,
        };
        unicoap_packet_t packet = { .remote = &remote };

        unicoap_messaging_process_rfc7252(buf, len,
                                          UNICOAP_MESSAGING_RFC7252_EVENT_RX, &packet);
    }

    return EXIT_SUCCESS;
}
