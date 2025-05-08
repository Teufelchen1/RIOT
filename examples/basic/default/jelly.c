#include <stdio.h>

#include "nanocbor/nanocbor.h"
#include "net/nanocoap.h"

static ssize_t _riot_version_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, coap_request_ctx_t *context)
{
    (void)context;
    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)RIOT_VERSION, strlen(RIOT_VERSION));
}

static ssize_t _riot_board_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len, coap_request_ctx_t *context)
{
    (void)context;
    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)RIOT_BOARD, strlen(RIOT_BOARD));
}

NANOCOAP_RESOURCE(riot_version) { \
    .path = "/riot/ver", .methods = COAP_GET, .handler = _riot_version_handler, .context = NULL \
};

NANOCOAP_RESOURCE(riot_board) { \
    .path = "/riot/board", .methods = COAP_GET, .handler = _riot_board_handler, .context = NULL \
};
