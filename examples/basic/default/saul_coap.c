#include <stdio.h>

#include "saul_reg.h"
#include "senml/saul.h"
#include "nanocbor/nanocbor.h"
#include "net/nanocoap.h"

static const char *_devname(saul_reg_t *dev) {
    if (dev->name == NULL) {
        return "(no name)";
    } else {
        return dev->name;
    }
}

/* this function does not check, if the given device is valid */
static void probe(int num, saul_reg_t *dev, nanocbor_encoder_t *enc)
{
    (void) num;
    int dim;
    phydat_t res;

    dim = saul_reg_read(dev, &res);
    if (dim <= 0) {
        //printf("error: failed to read from device #%i\n", num);
        return;
    }
    senml_saul_reg_encode_cbor(enc, dev);
}

static void list(nanocbor_encoder_t *enc)
{
    saul_reg_t *dev = saul_reg;
    assert(nanocbor_fmt_array_indefinite(enc) > 0);

    int i = 0;

    // if (dev) {
    //     printf("ID\tClass\t\tName\n");
    // }
    // else {
    //     printf("No devices found\n");
    // }
    while (dev) {
        nanocbor_fmt_array(enc, 3);
        nanocbor_fmt_uint(enc, i++);
        nanocbor_fmt_uint(enc, dev->driver->type);
        nanocbor_put_tstr(enc, _devname(dev));
        dev = dev->next;
    }
    nanocbor_fmt_end_indefinite(enc);
}

static void _reg_read(int num, nanocbor_encoder_t *enc)
{
    saul_reg_t *dev;

    // if (flash_strcmp(argv[2], "all") == 0) {
    //     probe_all();
    //     return;
    // }
    /* get device id */

    dev = saul_reg_find_nth(num);
    if (dev == NULL) {
        printf("error: undefined device id given\n");
        return;
    }
    probe(num, dev, enc);
}

static void _reg_write(int num, int data_src)
{
    int dim = 1;
    saul_reg_t *dev;
    phydat_t data;

    dev = saul_reg_find_nth(num);
    if (dev == NULL) {
        printf("error: undefined device given\n");
        return;
    }

    memset(&data, 0, sizeof(data));
    for (int i = 0; i < dim; i++) {
        data.val[i] = data_src;
    }

    /* write values to device */
    dim = saul_reg_write(dev, &data);
    if (dim <= 0) {
        if (dim == -ENOTSUP) {
            printf("error: device #%i is not writable\n", num);
        }
        else {
            printf("error: failure to write to device #%i\n", num);
        }
        return;
    }
}

ssize_t _saul_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                  coap_request_ctx_t *context)
{
    (void) context;
    uint8_t buffer[200];

    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buffer, sizeof(buffer));

    nanocbor_value_t decoder;
    nanocbor_value_t array;
    nanocbor_decoder_init(&decoder, pkt->payload, pkt->payload_len);
    nanocbor_enter_array(&decoder, &array);

    uint8_t command = 0;
    if (nanocbor_get_uint8(&array, &command) < 0) {
        return coap_reply_simple(pkt, COAP_CODE_UNPROCESSABLE_ENTITY, buf, len,
            COAP_FORMAT_NONE, NULL, 0);
    }

    if (command == 0) {
        list(&enc);
    } else {
        uint8_t id = 0;
        if (nanocbor_get_uint8(&array, &id) < 0) {
            return coap_reply_simple(pkt, COAP_CODE_UNPROCESSABLE_ENTITY, buf, len,
                COAP_FORMAT_NONE, NULL, 0);
        }
        if (command == 1) {
            // Read!
            _reg_read(id, &enc);
        }
        if (command == 2) {
            // Write!
            uint8_t data = 0;
            if (nanocbor_get_uint8(&array, &data) < 0) {
                return coap_reply_simple(pkt, COAP_CODE_UNPROCESSABLE_ENTITY, buf, len,
                    COAP_FORMAT_NONE, NULL, 0);
            }
            _reg_write(id, data);
        }
    }    

    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
        COAP_FORMAT_CBOR, buffer, nanocbor_encoded_len(&enc));
}

NANOCOAP_RESOURCE(saul_cbor) { \
    .path = "/jelly/Saul", .methods = COAP_POST, .handler = _saul_handler, .context = NULL \
};
