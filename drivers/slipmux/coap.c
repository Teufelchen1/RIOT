
#include "chunked_ringbuffer.h"
#include "checksum/crc16_ccitt.h"
#include "net/nanocoap.h"
#include "slipmux_internal.h"
#include "slipmux.h"

/* The special init is the result of normal fcs init combined with slipmux config start (0xa9) */
#define SPECIAL_INIT_FCS (0x374cU)

#define COAP_STACKSIZE (1024)
static char coap_stack[COAP_STACKSIZE];

static uint8_t buf[512];

void *_slipmux_coap_server_thread(void *arg)
{  
    slipmux_t *dev = arg;
    while (1) {
        thread_flags_wait_any(1);
        size_t len;
        while (crb_get_chunk_size(&dev->coap_rb, &len)) {
            if (len > sizeof(buf)) {
                continue;
            }
            crb_consume_chunk(&dev->coap_rb, buf, len);

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

            slipmux_lock();
            slipmux_write_byte(dev->config.uart, SLIPMUX_COAP_START);
            slipmux_write_bytes(dev->config.uart, buf, res);
            slipmux_write_bytes(dev->config.uart, (uint8_t *) &fcs_sum, 2);
            slipmux_write_byte(dev->config.uart, SLIPMUX_END);
            slipmux_unlock();
        }
    }

    return NULL;
}

void slipmux_coap_init(slipmux_t *dev) {
    crb_init(&dev->coap_rb, dev->coap_rx, sizeof(dev->coap_rx));

    dev->coap_server_pid = thread_create(coap_stack, sizeof(coap_stack), THREAD_PRIORITY_MAIN - 1,
                                     THREAD_CREATE_STACKTEST, _slipmux_coap_server_thread,
                                     (void *)dev, "Slipmux CoAP server");
}
