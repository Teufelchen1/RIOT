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

static bool _list_ids_type_name(nanocbor_encoder_t *enc)
{
    saul_reg_t *dev = saul_reg;
    if (nanocbor_fmt_array_indefinite(enc) < 0) {
        return false;
    }

    int i = 0;

    while (dev) {
        if (nanocbor_fmt_array(enc, 3) < 0) {
            return false;
        }
        if (nanocbor_fmt_uint(enc, i++) < 0) {
            return false;
        }
        if (nanocbor_fmt_uint(enc, dev->driver->type) < 0) {
            return false;
        }
        if (nanocbor_put_tstr(enc, _devname(dev)) < 0) {
            return false;
        }
        dev = dev->next;
    }

    if (nanocbor_fmt_end_indefinite(enc) < 0) {
        return false;
    }

    return true;
}

static int _read_saul_device_into_senml(int num, nanocbor_encoder_t *enc)
{
    saul_reg_t *dev = saul_reg_find_nth(num);

    if (dev == NULL) {
        return -ENODEV;
    }

    return senml_saul_reg_encode_cbor(enc, dev);
}

static int _reg_write(int num, int data_src)
{
    phydat_t data;
    int dim = 1;
    
    saul_reg_t *dev = saul_reg_find_nth(num);

    if (dev == NULL) {
        return -ENODEV;
    }

    /* Todo: actual care for the dimensions */
    for (int i = 0; i < dim; i++) {
        data.val[i] = data_src;
    }

    /* write values to device */
    return saul_reg_write(dev, &data);
}

int _saul_handler(unicoap_message_t* message, const unicoap_aux_t* aux,
    unicoap_request_context_t* ctx, void* arg)
{
    (void) arg;
    (void) aux;

    uint8_t *payload = unicoap_message_payload_get(message);
    size_t payload_len = unicoap_message_payload_get_size(message);

    /* should be enough to list ~10 sensors / actuators */
    uint8_t buffer[200];

    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buffer, sizeof(buffer));

    nanocbor_value_t decoder;
    nanocbor_value_t array;
    nanocbor_decoder_init(&decoder, payload, payload_len);

    switch (unicoap_request_get_method(message)) {
    default:
    case UNICOAP_METHOD_GET:
        UNICOAP_OPTIONS_ALLOC(options, 2);

        /* If we don't have a payload, list what we have available ... */ 
        if ((payload_len == 0) || (payload == NULL)) {
            if (!_list_ids_type_name(&enc)) {
                return UNICOAP_STATUS_INTERNAL_SERVER_ERROR;
            }

            if (unicoap_options_set_content_format(&options, UNICOAP_FORMAT_CBOR) < 0) {
                return UNICOAP_STATUS_INTERNAL_SERVER_ERROR;
            }

            unicoap_response_init_with_options(
                message, UNICOAP_STATUS_CONTENT, buffer, nanocbor_encoded_len(&enc), &options);

            return unicoap_send_response(message, ctx);
        }

        /* ... if we do have a payload, the user is requesting specific device(s) */
        /* the user might request one or more devices, so we start with an indefinite array */
        if(nanocbor_enter_array(&decoder, &array) != NANOCBOR_OK) {
            return UNICOAP_STATUS_BAD_REQUEST;
        }

        /* prepare the response array */
        if (nanocbor_fmt_array_indefinite(&enc) <= 0) {
            return UNICOAP_STATUS_INTERNAL_SERVER_ERROR;
        }

        /* while there are still devices left, that the user wants to read */
        while (!nanocbor_at_end(&array)) {
            uint8_t device_id = 0;
            /* get the device id */
            if (nanocbor_get_uint8(&array, &device_id) < 0) {
                return UNICOAP_STATUS_BAD_REQUEST;
            }
            /* read the specific device */
            int ret = _read_saul_device_into_senml(device_id, &enc);
            if (ret < 0) {
                return unicoap_response_status_from_errno(ret);
            }
        }

        /* finish the response array */
        if (nanocbor_fmt_end_indefinite(&enc) <= 0) {
            return UNICOAP_STATUS_BAD_REQUEST;
        }

        if (unicoap_options_set_content_format(&options, UNICOAP_FORMAT_SENML_CBOR) < 0) {
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
                return unicoap_response_status_from_errno(ret);
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
