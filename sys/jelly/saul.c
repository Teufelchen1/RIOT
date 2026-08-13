#include <stdio.h>

#include "saul_reg.h"
#include "senml/saul.h"
#include "nanocbor/nanocbor.h"
#include "net/unicoap.h"

static const char *_devname(saul_reg_t *dev) {
    if (dev->name == NULL) {
        return "(no name)";
    } else {
        return dev->name;
    }
}

/* this function does not check, if the given device is valid */
static int _probe(int num, saul_reg_t *dev, nanocbor_encoder_t *enc)
{
    (void) num;
    int dim;
    phydat_t res;

    dim = saul_reg_read(dev, &res);
    if (dim <= 0) {
        return dim;
    }

    senml_saul_reg_encode_cbor(enc, dev);
    return 1;
}

static void _list(nanocbor_encoder_t *enc)
{
    saul_reg_t *dev = saul_reg;
    assert(nanocbor_fmt_array_indefinite(enc) > 0);

    int i = 0;

    while (dev) {
        nanocbor_fmt_array(enc, 3);
        nanocbor_fmt_uint(enc, i++);
        nanocbor_fmt_uint(enc, dev->driver->type);
        nanocbor_put_tstr(enc, _devname(dev));
        dev = dev->next;
    }

    nanocbor_fmt_end_indefinite(enc);
}

static int _reg_read(int num, nanocbor_encoder_t *enc)
{
    saul_reg_t *dev;

    dev = saul_reg_find_nth(num);
    if (dev == NULL) {
        return -ENODEV;
    }
    return _probe(num, dev, enc);
}

static int _reg_write(int num, int data_src)
{
    int dim = 1;
    saul_reg_t *dev;
    phydat_t data;

    dev = saul_reg_find_nth(num);
    if (dev == NULL) {
        return -ENODEV;
    }

    memset(&data, 0, sizeof(data));
    for (int i = 0; i < dim; i++) {
        data.val[i] = data_src;
    }

    /* write values to device */
    dim = saul_reg_write(dev, &data);
    return dim;
}

static int _convert_errorno_to_coap_code(int num)
{
    switch (num) {
    case -EINVAL:
        return UNICOAP_STATUS_UNPROCESSABLE_ENTITY;
    case -EBADMSG:
        return UNICOAP_STATUS_BAD_REQUEST;
    case -ECANCELED:
    case -EIO:
        return UNICOAP_STATUS_INTERNAL_SERVER_ERROR;
    case -ENODEV:
        return UNICOAP_STATUS_PATH_NOT_FOUND;
    case -ENOTSUP:
        return UNICOAP_STATUS_METHOD_NOT_ALLOWED;
    default:
        if (num < 0) {
            return UNICOAP_STATUS_INTERNAL_SERVER_ERROR;
        } 
        return UNICOAP_STATUS_CONTENT;
    }
}

int _saul_handler(unicoap_message_t* message, const unicoap_aux_t* aux,
    unicoap_request_context_t* ctx, void* arg)
{
    (void) arg;
    (void) aux;

    uint8_t *payload = unicoap_message_payload_get(message);
    size_t payload_len = unicoap_message_payload_get_size(message);

    uint8_t buffer[200];

    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buffer, sizeof(buffer));

    nanocbor_value_t decoder;
    nanocbor_value_t array;
    nanocbor_decoder_init(&decoder, payload, payload_len);

    switch (unicoap_request_get_method(message)) {
    default:
    case UNICOAP_METHOD_GET:
        if(nanocbor_enter_array(&decoder, &array) == NANOCBOR_OK) {
            uint8_t id = 0;
            if (nanocbor_fmt_array_indefinite(&enc) <= 0) {
                return UNICOAP_STATUS_BAD_REQUEST;
            }
            while (!nanocbor_at_end(&array)) {
                if (nanocbor_get_uint8(&array, &id) < 0) {
                    return UNICOAP_STATUS_BAD_REQUEST;
                }
                int ret = _reg_read(id, &enc);
                if (ret < 0) {
                    return _convert_errorno_to_coap_code(ret);
                }
            }
            if (nanocbor_fmt_end_indefinite(&enc) <= 0) {
                return UNICOAP_STATUS_BAD_REQUEST;
            }
        } else {
            _list(&enc);
        }

        UNICOAP_OPTIONS_ALLOC(options, 2);

        if (unicoap_options_set_content_format(&options, UNICOAP_FORMAT_CBOR) < 0) {
            return UNICOAP_STATUS_INTERNAL_SERVER_ERROR;
        }

        unicoap_response_init_with_options(message, UNICOAP_STATUS_CONTENT, buffer, nanocbor_encoded_len(&enc), &options);
        return unicoap_send_response(message, ctx);
    case UNICOAP_METHOD_PUT:
    case UNICOAP_METHOD_POST:
    case UNICOAP_METHOD_PATCH:
        if(nanocbor_enter_array(&decoder, &array) == NANOCBOR_OK) {
            uint8_t id = 0;
            if (nanocbor_get_uint8(&array, &id) < 0) {
                return UNICOAP_STATUS_BAD_REQUEST;
            }
            uint8_t data = 0;
            if (nanocbor_get_uint8(&array, &data) < 0) {
                return UNICOAP_STATUS_BAD_REQUEST;
            }
            int ret = _reg_write(id, data);
            if (ret < 0) {
                return _convert_errorno_to_coap_code(ret);
            }
        } else {
            return UNICOAP_STATUS_BAD_REQUEST;
        }
        break;
    }

    return UNICOAP_STATUS_CHANGED;
}

UNICOAP_RESOURCE(saul_cbor) {
    .path = UNICOAP_PATH("jelly", "saul"),
    .methods = UNICOAP_METHODS(UNICOAP_METHOD_GET, UNICOAP_METHOD_PUT, UNICOAP_METHOD_POST, UNICOAP_METHOD_PATCH),
    .handler = _saul_handler,
    .protocols = UNICOAP_PROTOCOLS(UNICOAP_PROTO_SLIPMUX),
};
