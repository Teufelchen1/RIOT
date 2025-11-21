#include "chunked_ringbuffer.h"
#include "checksum/crc16_ccitt.h"
#include "event.h"
#include "net/nanocoap.h"
#include "net/unicoap/transport.h"

#include "slipmux_internal.h"
#include "slipmux.h"

/* The special init is the result of normal fcs init combined with slipmux config start (0xa9) */
#define SPECIAL_INIT_FCS (0x374cU)

static event_queue_t *queue = NULL;

/* called in ISR context */
void _slipmux_coap_dispatch_recv(event_t *event) {
    if (queue) {
        event_post(queue, event);
    }
}

void slipmux_coap_set_event_queue(event_queue_t *q) {
    queue = q;
}

int slipmux_coap_recv(uint8_t *buf, size_t buf_size, slipmux_t *dev) {
    size_t len;
    if (crb_get_chunk_size(&dev->coap_rb, &len)) {
        if ((len > buf_size) || (len <= 2)) {
            return -1;
        }
        crb_consume_chunk(&dev->coap_rb, buf, len);

        /* Is the crc correct via residue(=0xF0B8) test */
        if (crc16_ccitt_fcs_update(SPECIAL_INIT_FCS, buf, len) != 0xF0B8) {
            return -1;
        }

        /* cut off the FCS checksum at the end */
        size_t pktlen = len - 2;

        return pktlen;
    }
    return 0;
}

void slipmux_coap_send(uint8_t *buf, size_t len, const slipmux_t *dev) {
    uint16_t fcs_sum = crc16_ccitt_fcs_finish(SPECIAL_INIT_FCS, buf, len);

    slipmux_lock();
    slipmux_write_byte(dev->config.uart, SLIPMUX_COAP_START);
    slipmux_write_bytes(dev->config.uart, buf, len);
    slipmux_write_bytes(dev->config.uart, (uint8_t *) &fcs_sum, 2);
    slipmux_write_byte(dev->config.uart, SLIPMUX_END);
    slipmux_unlock();
}

void slipmux_coap_init(slipmux_t *dev, unsigned index) {
    (void) index;
    dev->event.handler = unicoap_slipmux_recv_handler;
    crb_init(&dev->coap_rb, dev->coap_rx, sizeof(dev->coap_rx));
}
