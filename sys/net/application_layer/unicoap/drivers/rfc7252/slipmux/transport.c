/*
 * Copyright (C) 2024-2025 Carl Seifert
 * Copyright (C) 2024-2025 TU Dresden
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
 */

/**
 * @file
 * @ingroup net_unicoap_drivers_udp
 * @brief   Transport implementation of CoAP over UDP driver
 * @author  Carl Seifert <carl.seifert@tu-dresden.de>
 */

#include <stdint.h>
#include <errno.h>
#include "architecture.h"
#include "checksum/crc16_ccitt.h"
#include "net/unicoap/transport.h"

#include "slipmux.h"

//#define ENABLE_DEBUG CONFIG_UNICOAP_DEBUG_LOGGING
#define ENABLE_DEBUG 1
#include "debug.h"
#include "private.h"

extern int unicoap_messaging_process_rfc7252(const uint8_t* pdu, size_t size, bool truncated,
                                             unicoap_packet_t* packet);

static uint8_t buf[512];

void unicoap_slipmux_recv_handler(event_t * event) {
    slipmux_t *dev = container_of(event, slipmux_t, event);

    unicoap_endpoint_t remote = {
        .proto = UNICOAP_PROTO_SLIPMUX,
        .slipmux_ep = dev,
    };

    unicoap_packet_t packet = { .remote = &remote };
    while (1) {
        ssize_t len = slipmux_coap_recv(buf, sizeof(buf), dev);
        if (len <= 0) {
            return;
        }
        unicoap_messaging_process_rfc7252(buf, len, false, &packet);
    }
}

int unicoap_transport_sendv_slipmux(iolist_t * iolist, const unicoap_endpoint_t* remote) {
    DEBUG("Sending slipmux coap to UART %d\n", remote->slipmux_ep->config.uart);
    size_t len = iolist_to_buffer(iolist, buf, sizeof(buf));
    slipmux_coap_send(buf, len, remote->slipmux_ep);

    return 0;
}

int unicoap_init_slipmux(event_queue_t* queue)
{
    slipmux_coap_set_event_queue(queue);
    return 0;
}

/*
sock_udp_t* unicoap_transport_udp_get_socket(void)
{
    return &_udp_socket;
}

int unicoap_transport_udp_add_socket(sock_udp_t* socket, sock_udp_ep_t* local) {
    return _add_socket(sock_udp_get_async_ctx(&_udp_socket)->queue, socket, local);
}

int unicoap_transport_udp_remove_socket(sock_udp_t* socket) {
    sock_udp_close(socket);
    return 0;
}

int unicoap_deinit_udp(event_queue_t* queue)
{
    (void)queue;
    sock_udp_close(&_udp_socket);
    return 0;
}
*/