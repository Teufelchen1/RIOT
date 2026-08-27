#include <stdio.h>

#include "nanocbor/nanocbor.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netif/ipv6.h"
#include "net/ipv6/addr.h"
#include "net/unicoap.h"

#define TAG_IEEE_MAC 48 /* Choosen by IANA */
#define TAG_IPV6 54 /* Choosen by IANA */

#define TAG_NETIF_NAME 0
#define TAG_NETOPT_ADDRESS 1
#define TAG_NETOPT_ADDRESS_LONG 2
#define TAG_NETOPT_IS_WIRED 3

#define TAG_NETOPT_IPV6_ADDR 4
#define TAG_NETOPT_IPV6_GROUP 5

#define TAG_NETOPT_CHANNEL 6
#define TAG_NETOPT_CHANNEL_FREQUENCY 7
#define TAG_NETOPT_CHANNEL_PAGE 8
#define TAG_NETOPT_NID 9
#define TAG_NETOPT_RSSI 10
#define TAG_NETOPT_CCA_THRESHOLD 11
#define TAG_NETOPT_LINK 12
#define TAG_NETOPT_ACTIVE 13
#define TAG_NETOPT_TX_POWER 14
#define TAG_NETOPT_STATE 15
#define TAG_NETOPT_RETRANS 16
#define TAG_NETOPT_CSMA_RETRIES 17
#define TAG_NETOPT_MAX_PDU_SIZE 18
#define TAG_NETOPT_MAX_PDU_SIZE_IPV6 19
#define TAG_NETOPT_HOP_LIMIT 20
#define TAG_NETOPT_SRC_LEN 21

#define TAG_FLAG_ARRAY 22
#define TAG_NETOPT_PROMISCUOUSMODE 0
#define TAG_NETOPT_AUTOACK 1
#define TAG_NETOPT_ACK_REQ 2
#define TAG_NETOPT_PRELOADING 3
#define TAG_NETOPT_RAWMODE 4
#define TAG_NETOPT_MAC_NO_SLEEP 5
#define TAG_NETOPT_CSMA 6
#define TAG_NETOPT_AUTOCCA 7
#define TAG_NETOPT_IQ_INVERT 8
#define TAG_NETOPT_SINGLE_RECEIVE 9
#define TAG_NETOPT_CHANNEL_HOP 10
#define TAG_NETOPT_OTAA 11
#define TAG_NETOPT_IPV6_FORWARDING 12
#define TAG_NETOPT_IPV6_SND_RTR_ADV 13
#define TAG_NETOPT_6LO 14
#define TAG_NETOPT_6LO_ABR 15
#define TAG_NETOPT_6LO_IPHC 16

#define TAG_IEEE802154_ARRAY 23
#define TAG_NETOPT_IEEE802154_PHY 0
#define TAG_NETOPT_OQPSK_RATE 1
#define TAG_NETOPT_MR_OQPSK_CHIPS 2
#define TAG_NETOPT_MR_OQPSK_RATE 3
#define TAG_NETOPT_MR_OFDM_OPTION 4
#define TAG_NETOPT_MR_OFDM_MCS 5
#define TAG_NETOPT_MR_FSK_MODULATION_INDEX 6
#define TAG_NETOPT_MR_FSK_MODULATION_ORDER 7
#define TAG_NETOPT_MR_FSK_SRATE 8
#define TAG_NETOPT_MR_FSK_FEC 9
#define TAG_NETOPT_CHANNEL_SPACING 10

#define TAG_LORA_ARRAY 24
#define TAG_NETOPT_BANDWIDTH 0
#define TAG_NETOPT_SPREADING_FACTOR 1
#define TAG_NETOPT_CODING_RATE 2
#define TAG_NETOPT_DEMOD_MARGIN 3
#define TAG_NETOPT_NUM_GATEWAYS 4

#define TAG_u32(enc, num, data) TAG_UINT(enc, num, data)
#define TAG_u16(enc, num, data) TAG_UINT(enc, num, data)
#define TAG_u8(enc, num, data) TAG_UINT(enc, num, data)
#define TAG_i16(enc, num, data) TAG_INT(enc, num, data)
#define TAG_enabled(enc, num, data) TAG_BOOL(enc, num, (data == NETOPT_ENABLE))
#define TAG_state(enc, num, data) TAG_UINT(enc, num, data)

#define CAT(A,B) CAT_(A,B)
#define CAT_(A,B) A##B

#define TAG_NUM(type) TAG_ ## type

#define TAG_PRE_DISABLE(res, netif, enc, netopt, data) \
    data = NETOPT_DISABLE;\
    TAG(res, netif, enc, netopt, data)

#define TAG(res, netif, enc, netopt, data) do {\
    res = netif_get_opt(netif, netopt, 0, &data, sizeof(data));\
    if (res >= 0) { \
        CAT(TAG_, data)(enc, TAG_NUM(netopt), data); \
    }\
} while(0)

#define TAG_UINT(encoder, number, payload) do {     \
    assert(nanocbor_fmt_uint(encoder, number) > 0);  \
    assert(nanocbor_fmt_uint(encoder, payload) > 0);\
} while(0)

#define TAG_INT(encoder, number, payload) do {     \
    assert(nanocbor_fmt_uint(encoder, number) > 0); \
    assert(nanocbor_fmt_int(encoder, payload) > 0);\
} while(0)

#define TAG_BOOL(encoder, number, payload) do {     \
    assert(nanocbor_fmt_uint(encoder, number) > 0); \
    assert(nanocbor_fmt_bool(encoder, payload) > 0);\
} while(0)


#define GET_u8(dec, data) nanocbor_get_uint8(dec, &data) < 0
#define GET_u16(dec, data) nanocbor_get_uint16(dec, &data) < 0
#define GET_i16(dec, data) nanocbor_get_int16(dec, &data) < 0

#define SET(ret, netif, dec, netopt, data) \
    case TAG_NUM(netopt):\
        if (CAT(GET_, data)(dec, data)) { \
            return UNICOAP_STATUS_BAD_REQUEST; \
        } \
        if ((ret = netif_set_opt(netif, netopt, 0, &data, sizeof(data))) < 0) { \
            return unicoap_response_status_from_errno(ret); \
        }\
        break

static void _netif_list(netif_t *iface, nanocbor_encoder_t *enc);

static int _set_link_up_down(nanocbor_value_t *decoder, netif_t *netif, unicoap_method_t method)
{
    if (method == UNICOAP_METHOD_PATCH) {
        bool netopt_change = false;
        if (nanocbor_get_bool(decoder, &netopt_change) != NANOCBOR_OK) {
            return -EBADMSG;
        }
        netopt_enable_t en_or_dis = netopt_change ? NETOPT_ENABLE : NETOPT_DISABLE;
#if IS_USED(MODULE_LWIP_NETIF) /* lwIP sets netif state, not link state */
        if (netif_set_opt(netif, NETOPT_ACTIVE, 0, &en_or_dis, sizeof(en_or_dis)) < 0) {
            return -EIO;
        }
#else
        if (netif_set_opt(netif, NETOPT_LINK, 0, &en_or_dis, sizeof(en_or_dis)) < 0) {
            return -EIO;
        }
#endif
    } else {
        return -EINVAL;
    }
    return 1;
}

static int _get_ipv6_from_cbor(nanocbor_value_t *decoder, ipv6_addr_t **ipv6_buffer)
{
    size_t ipv6_buffer_len;
    if (nanocbor_get_bstr(decoder, (const uint8_t **) ipv6_buffer, &ipv6_buffer_len) != NANOCBOR_OK) {
        return -EBADMSG;
    }
    if (ipv6_buffer_len != 16) {
        return -EINVAL;
    }

    return 1;
}

static int _set_ipv6(nanocbor_value_t *decoder, netif_t *netif, unicoap_method_t method)
{
    int ret = 0;
    if (method == UNICOAP_METHOD_PATCH) {
        ipv6_addr_t *ipv6_addr;
        if ((ret = _get_ipv6_from_cbor(decoder, &ipv6_addr)) < 0) {
            return ret;
        }

        if (ipv6_addr_is_multicast(ipv6_addr)) {
            if ((ret = netif_set_opt(netif, NETOPT_IPV6_GROUP_LEAVE, 0, ipv6_addr,
                              sizeof(ipv6_addr_t)) < 0)) {
                return ret;
            }
        }
        else {
            if ((ret = netif_set_opt(netif, NETOPT_IPV6_ADDR_REMOVE, 0, ipv6_addr,
                              sizeof(ipv6_addr_t)) < 0)) {
                return ret;
            }
        }
    }
    if ((method == UNICOAP_METHOD_POST) || (method == UNICOAP_METHOD_PUT)) {
        nanocbor_value_t inner_array;
        if(nanocbor_enter_array(decoder, &inner_array) != NANOCBOR_OK) {
            return -EBADMSG;
        }

        ipv6_addr_t *ipv6_addr;
        if ((ret = _get_ipv6_from_cbor(&inner_array, &ipv6_addr)) < 0) {
            return ret;
        }

        uint8_t prefix = 128;
        if (nanocbor_get_uint8(&inner_array, &prefix) < 0) {
            return -EBADMSG;
        }

        uint16_t flags = GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID | (prefix << 8);
        if ((ret = netif_set_opt(netif,
            NETOPT_IPV6_ADDR, flags, ipv6_addr, sizeof(ipv6_addr_t))) < 0) {
            return ret;
        }
    }
    return 1;
}

static unicoap_status_t _gnrc_netif_set(uint32_t tag, nanocbor_value_t *cbor_value, netif_t *netif, unicoap_method_t method)
{
    //uint32_t u32;
    uint16_t u16;
    int16_t i16;
    uint8_t u8;
    int ret = 0;

    switch (tag) {
    case TAG_NETOPT_LINK:
        if ((ret = _set_link_up_down(cbor_value, netif, method)) < 0) {
            return unicoap_response_status_from_errno(ret);
        }
        break;
    case TAG_IPV6:
        if ((ret = _set_ipv6(cbor_value, netif, method)) < 0) {
            return unicoap_response_status_from_errno(ret);
        }
        break;
//     if ((strcmp("addr", key) == 0) || (strcmp("addr_short", key) == 0)) {
//         return _netif_set_addr(iface, NETOPT_ADDRESS, value);
//     }
    case TAG_NETOPT_ADDRESS_LONG:
        const uint8_t * hwaddr = NULL;
        size_t len = 0;
        if (nanocbor_get_bstr(cbor_value, &hwaddr, &len) != NANOCBOR_OK) {
            return UNICOAP_STATUS_BAD_REQUEST;
        }
        if ((ret = netif_set_opt(netif, NETOPT_ADDRESS_LONG, 0, (void *)hwaddr, len)) < 0) {
            return unicoap_response_status_from_errno(ret);
        }
        break;
    SET(ret, netif, cbor_value, NETOPT_CCA_THRESHOLD, u8);
    SET(ret, netif, cbor_value, NETOPT_CHANNEL_FREQUENCY, u8);
#if IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORA)
    // else if ((strcmp("bandwidth", key) == 0) || (strcmp("bw", key) == 0)) {
    //     return _netif_set_bandwidth(iface, value);
    // }
    SET(ret, netif, cbor_value, NETOPT_SPREADING_FACTOR, u8);
    // else if ((strcmp("coding_rate", key) == 0) || (strcmp("cr", key) == 0)) {
    //     return _netif_set_coding_rate(iface, value);
    // }
#endif  /* MODULE_SHELL_CMD_GNRC_NETIF_LORA */
// #if IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORAWAN)
// #if IS_USED(MODULE_GNRC_LORAWAN_1_1)
//     else if (strcmp("joineui", key) == 0) {
//         return _netif_set_lw_key(iface, NETOPT_LORAWAN_JOINEUI, value);
//     }
//     else if (strcmp("fnwksintkey", key) == 0) {
//         return _netif_set_lw_key(iface, NETOPT_LORAWAN_FNWKSINTKEY, value);
//     }
//     else if (strcmp("snwksintkey", key) == 0) {
//         return _netif_set_lw_key(iface, NETOPT_LORAWAN_SNWKSINTKEY, value);
//     }
//     else if (strcmp("nwksenckey", key) == 0) {
//         return _netif_set_lw_key(iface, NETOPT_LORAWAN_NWKSENCKEY, value);
//     }
//     else if (strcmp("nwkkey", key) == 0) {
//         return _netif_set_lw_key(iface, NETOPT_LORAWAN_NWKKEY, value);
//     }
// #else
//     else if (strcmp("appeui", key) == 0) {
//         return _netif_set_lw_key(iface, NETOPT_LORAWAN_APPEUI, value);
//     }
//     else if (strcmp("nwkskey", key) == 0) {
//         return _netif_set_addr(iface, NETOPT_LORAWAN_NWKSKEY, value);
//     }
// #endif /* IS_USED(MODULE_GNRC_LORAWAN_1_1) */
//     else if (strcmp("appskey", key) == 0) {
//         return _netif_set_addr(iface, NETOPT_LORAWAN_APPSKEY, value);
//     }
//     else if (strcmp("appkey", key) == 0) {
//         return _netif_set_lw_key(iface, NETOPT_LORAWAN_APPKEY, value);
//     }
//     else if (strcmp("deveui", key) == 0) {
//         return _netif_set_addr(iface, NETOPT_ADDRESS_LONG, value);
//     }

//     else if (strcmp("dr", key) == 0) {
//         return _netif_set_u8(iface, NETOPT_LORAWAN_DR, 0, value);
//     }
//     else if (strcmp("rx2_dr", key) == 0) {
//         return _netif_set_u8(iface, NETOPT_LORAWAN_RX2_DR, 0, value);
//     }
// #endif /* MODULE_SHELL_CMD_GNRC_NETIF_LORAWAN */
// #ifdef MODULE_NETDEV_IEEE802154_MULTIMODE
//     else if ((strcmp("phy_mode", key) == 0) || (strcmp("phy", key) == 0)) {
//         return _netif_set_ieee802154_phy_mode(iface, value);
//     }
// #endif /* MODULE_NETDEV_IEEE802154_MULTIMODE */
#ifdef MODULE_NETDEV_IEEE802154_OQPSK
    SET(ret, netif, cbor_value, NETOPT_OQPSK_RATE, u8);
#endif /* MODULE_NETDEV_IEEE802154_OQPSK */
#ifdef MODULE_NETDEV_IEEE802154_MR_OQPSK
    SET(ret, netif, cbor_value, NETOPT_MR_OQPSK_CHIPS, u16);
    SET(ret, netif, cbor_value, NETOPT_MR_OQPSK_RATE, u8);
#endif /* MODULE_NETDEV_IEEE802154_MR_OQPSK */
#ifdef MODULE_NETDEV_IEEE802154_MR_OFDM
    SET(ret, netif, cbor_value, NETOPT_MR_OFDM_OPTION, u8);
    SET(ret, netif, cbor_value, NETOPT_MR_OFDM_MCS, u8);
#endif /* MODULE_NETDEV_IEEE802154_MR_OFDM */
#ifdef MODULE_NETDEV_IEEE802154_MR_FSK
//     else if ((strcmp("modulation_index", key) == 0) || (strcmp("midx", key) == 0)) {
//         return _netif_set_fsk_modulation_index(iface, value);
//     }
    SET(ret, netif, cbor_value, NETOPT_MR_FSK_MODULATION_ORDER, u8);
    SET(ret, netif, cbor_value, NETOPT_MR_FSK_SRATE, u16);
//     else if ((strcmp("forward_error_correction", key) == 0) || (strcmp("fec", key) == 0)) {
//         return _netif_set_fsk_fec(iface, value);
//     }
    SET(ret, netif, cbor_value, NETOPT_CHANNEL_SPACING, u16);
#endif /* MODULE_NETDEV_IEEE802154_MR_FSK */
    SET(ret, netif, cbor_value, NETOPT_CHANNEL, u16);
    SET(ret, netif, cbor_value, NETOPT_CSMA_RETRIES, u8);
    SET(ret, netif, cbor_value, NETOPT_HOP_LIMIT, u8);
//     else if (strcmp("key", key) == 0) {
//         return _netif_set_encrypt_key(iface, NETOPT_ENCRYPTION_KEY, value);
//     }
// #ifdef MODULE_GNRC_IPV6
//     else if (strcmp("mtu", key) == 0) {
//         return _netif_set_u16(iface, NETOPT_MAX_PDU_SIZE, GNRC_NETTYPE_IPV6,
//                               value);
//     }
// #endif
    SET(ret, netif, cbor_value, NETOPT_NID, u16);
    SET(ret, netif, cbor_value, NETOPT_CHANNEL_PAGE, u16);
    SET(ret, netif, cbor_value, NETOPT_TX_POWER, i16);
//     else if (strcmp("retrans", key) == 0) {
//         return _netif_set_u8(iface, NETOPT_RETRANS, 0, value);
//     }
//     else if (strcmp("src_len", key) == 0) {
//         return _netif_set_u16(iface, NETOPT_SRC_LEN, 0, value);
//     }
//     else if (strcmp("state", key) == 0) {
//         return _netif_set_state(iface, value);
//     }

    default:
        return UNICOAP_STATUS_UNPROCESSABLE_ENTITY;
    }
    return UNICOAP_STATUS_CHANGED;
}

static void _list_all_netif_names(nanocbor_encoder_t *enc)
{
    assert(nanocbor_fmt_array_indefinite(enc) > 0);

    char name[CONFIG_NETIF_NAMELENMAX];
    netif_t *last = NULL;

    /* Get interfaces in reverse order since the list is used like a stack.
     * Stop when first netif in list already has been listed. */
    while (last != netif_iter(NULL)) {
        netif_t *netif = NULL;
        netif_t *next = netif_iter(netif);
        /* Step until next is end of list or was previously listed. */
        do {
            netif = next;
            next = netif_iter(netif);
        } while (next && next != last);

        netif_get_name(netif, name);
        /* safe: netif_get_name promises to be null terminated */
        assert(nanocbor_put_tstr(enc, name) == NANOCBOR_OK);
        last = netif;
    }

    assert(nanocbor_fmt_end_indefinite(enc) > 0);
}

static bool _get_netif_from_query(unicoap_message_t* message, netif_t **netif)
{
    const char* netif_name = NULL;

    int netif_name_length = unicoap_options_get_first_uri_query_by_name_string(message->options, "name", &netif_name);
    
    if (netif_name_length <= 0) {
        /* No query found */
        return false;
    }

    /* Query found */
    *netif = netif_get_by_name_buffer(netif_name, netif_name_length);
    return true;
}


int _gnrc_netif_handler(unicoap_message_t* message, const unicoap_aux_t* aux,
    unicoap_request_context_t* ctx, void* arg)
{
    (void) arg;
    (void) aux;

    uint8_t buffer[256];
    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buffer, sizeof(buffer));

    netif_t *netif = NULL;

    if (_get_netif_from_query(message, &netif)) {
        /* There was a query for a netif but that netif didn't exist */
        if (!netif)
            return UNICOAP_STATUS_PATH_NOT_FOUND;
    }
    
    unicoap_method_t method = unicoap_request_get_method(message);

    /* Read-only operations */
    if (method == UNICOAP_METHOD_GET) {
        if (netif) {
            _netif_list(netif, &enc);
        } else {
            _list_all_netif_names(&enc);
        }

        UNICOAP_OPTIONS_ALLOC(options, 2);

        if (unicoap_options_set_content_format(&options, UNICOAP_FORMAT_CBOR) < 0) {
            return UNICOAP_STATUS_INTERNAL_SERVER_ERROR;
        }

        unicoap_response_init_with_options(message, UNICOAP_STATUS_CONTENT, buffer, nanocbor_encoded_len(&enc), &options);
        return unicoap_send_response(message, ctx);
    }

    /* PUT/POST/PATCH: Must have valid interface to work with */
    if (!netif)
        return UNICOAP_STATUS_BAD_REQUEST;

    /* Setup the decoding of the cbor payload */
    uint8_t *payload = unicoap_message_payload_get(message);
    size_t payload_len = unicoap_message_payload_get_size(message);
    nanocbor_value_t decoder;
    nanocbor_decoder_init(&decoder, payload, payload_len);

    /* Payload must be in an array, even though we only expect one item in it */
    nanocbor_value_t outer_cbor_array;
    if (nanocbor_enter_array(&decoder, &outer_cbor_array) != NANOCBOR_OK) {
        return UNICOAP_STATUS_BAD_REQUEST;
    }

    /* Get our task */
    uint32_t tag = 0;
    if (nanocbor_get_tag(&outer_cbor_array, &tag) != NANOCBOR_OK) {
        return UNICOAP_STATUS_BAD_REQUEST;
    }

    return _gnrc_netif_set(tag, &outer_cbor_array, netif, method);
}

UNICOAP_RESOURCE(netif_cbor) {
  .path = UNICOAP_PATH("jelly", "netif"),
  .handler = _gnrc_netif_handler,
  .methods = UNICOAP_METHODS(UNICOAP_METHOD_GET, UNICOAP_METHOD_PUT, UNICOAP_METHOD_POST, UNICOAP_METHOD_PATCH),
  .protocols = UNICOAP_PROTOCOLS(UNICOAP_PROTO_SLIPMUX),
};

static void _netif_list(netif_t *iface, nanocbor_encoder_t *enc)
{
#if IS_USED(MODULE_IPV6)
    ipv6_addr_t ipv6_addrs[MAX(CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF, GNRC_NETIF_IPV6_GROUPS_NUMOF)];
#endif
    uint8_t hwaddr[GNRC_NETIF_L2ADDR_MAXLEN];
    uint32_t u32;
    uint16_t u16;
    int16_t i16;
    uint8_t u8;
    netopt_enable_t enabled;
    netopt_state_t state;
    int res;

    assert(iface);
    assert(enc);

    assert(nanocbor_fmt_map_indefinite(enc) > 0);

    char name[CONFIG_NETIF_NAMELENMAX];

    netif_get_name(iface, name);

    assert(nanocbor_fmt_uint(enc, TAG_NETIF_NAME) > 0);
    /* safe: netif_get_name promises to be null terminated */
    assert(nanocbor_put_tstr(enc, name) == NANOCBOR_OK);

    res = netif_get_opt(iface, NETOPT_ADDRESS, 0, hwaddr, sizeof(hwaddr));
    if (res > 0) {
        assert((unsigned) res <= sizeof(hwaddr));
        if (res == 6) {
            assert(nanocbor_fmt_uint(enc, TAG_NETOPT_ADDRESS) > 0);
            /* Tag 48 is set by iana as IEEE MAC, RFC9542 */
            assert(nanocbor_fmt_tag(enc, TAG_IEEE_MAC) > 0);
            assert(nanocbor_put_bstr(enc, hwaddr, 6) == NANOCBOR_OK);
        }
    } else {
        /* todo */
    }
    TAG(res, iface, enc, NETOPT_CHANNEL, u16);   
    TAG(res, iface, enc, NETOPT_CHANNEL_FREQUENCY, u32);
    TAG(res, iface, enc, NETOPT_CHANNEL_PAGE, u16);
    TAG(res, iface, enc, NETOPT_NID, u16);
    TAG(res, iface, enc, NETOPT_RSSI, i16);

#if IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORA) || IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORAWAN)
    assert(nanocbor_fmt_uint(enc, TAG_LORA_ARRAY) > 0);
    assert(nanocbor_fmt_map_indefinite(enc) > 0);
#if IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORA)
    TAG(res, iface, enc, NETOPT_BANDWIDTH, u8);
    TAG(res, iface, enc, NETOPT_SPREADING_FACTOR, u8);
    TAG(res, iface, enc, NETOPT_CODING_RATE, u8);
#endif /* MODULE_SHELL_CMD_GNRC_NETIF_LORA */
#if IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORAWAN)
    TAG(res, iface, enc, NETOPT_DEMOD_MARGIN, u8);
    TAG(res, iface, enc, NETOPT_NUM_GATEWAYS, u8);
#endif /* MODULE_SHELL_CMD_GNRC_NETIF_LORAWAN */
    assert(nanocbor_fmt_end_indefinite(enc) > 0);
#endif /* MODULE_SHELL_CMD_GNRC_NETIF_LORA || MODULE_SHELL_CMD_GNRC_NETIF_LORAWAN */
#ifdef MODULE_NETDEV_IEEE802154
    res = netif_get_opt(iface, NETOPT_IEEE802154_PHY, 0, &u8, sizeof(u8));
    if (res >= 0) {
        assert(nanocbor_fmt_uint(enc, TAG_IEEE802154_ARRAY) > 0);
        assert(nanocbor_fmt_map_indefinite(enc) > 0);

        assert(nanocbor_fmt_uint(enc, TAG_NETOPT_IEEE802154_PHY) > 0);
        assert(nanocbor_fmt_uint(enc, u8) > 0);
        switch (u8) {
#ifdef MODULE_NETDEV_IEEE802154_OQPSK
        case IEEE802154_PHY_OQPSK:
            TAG(res, iface, enc, NETOPT_OQPSK_RATE, u8);
            break;

#endif /* MODULE_NETDEV_IEEE802154_OQPSK */
#ifdef MODULE_NETDEV_IEEE802154_MR_OQPSK
        case IEEE802154_PHY_MR_OQPSK:
            TAG(res, iface, enc, NETOPT_MR_OQPSK_CHIPS, u16);
            TAG(res, iface, enc, NETOPT_MR_OQPSK_RATE, u8);
            break;

#endif /* MODULE_NETDEV_IEEE802154_MR_OQPSK */
#ifdef MODULE_NETDEV_IEEE802154_MR_OFDM
        case IEEE802154_PHY_MR_OFDM:
            TAG(res, iface, enc, NETOPT_MR_OFDM_OPTION, u8);
            TAG(res, iface, enc, NETOPT_MR_OFDM_MCS, u8);
            break;
#endif /* MODULE_NETDEV_IEEE802154_MR_OFDM */
#ifdef MODULE_NETDEV_IEEE802154_MR_FSK
        case IEEE802154_PHY_MR_FSK:
            TAG(res, iface, enc, NETOPT_MR_FSK_MODULATION_INDEX, u8);
            TAG(res, iface, enc, NETOPT_MR_FSK_MODULATION_ORDER, u8);
            TAG(res, iface, enc, NETOPT_MR_FSK_SRATE, u16);
            TAG(res, iface, enc, NETOPT_MR_FSK_FEC, u8);
            TAG(res, iface, enc, NETOPT_CHANNEL_SPACING, u16);
            break;
#endif /* MODULE_NETDEV_IEEE802154_MR_FSK */
        }
        assert(nanocbor_fmt_end_indefinite(enc) > 0);
    }
#endif /* MODULE_NETDEV_IEEE802154 */

    TAG(res, iface, enc, NETOPT_LINK, enabled);
#if IS_USED(MODULE_LWIP_NETIF) /* only supported on lwIP for now */
    TAG(res, iface, enc, NETOPT_ACTIVE, enabled);
#endif /* MODULE_LWIP_NETIF */

    res = netif_get_opt(iface, NETOPT_ADDRESS_LONG, 0, hwaddr, sizeof(hwaddr));
    if (res == 8) {
        assert(nanocbor_fmt_uint(enc, TAG_NETOPT_ADDRESS_LONG) > 0);
        /* 48 is set by iana as IEEE EUI, RFC9542 */
        assert(nanocbor_fmt_tag(enc, TAG_IEEE_MAC) > 0);
        assert(nanocbor_put_bstr(enc, hwaddr, sizeof(hwaddr)) == NANOCBOR_OK);
    }
    TAG(res, iface, enc, NETOPT_TX_POWER, i16);
    TAG(res, iface, enc, NETOPT_STATE, state);
    TAG(res, iface, enc, NETOPT_RETRANS, u8);
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_CSMA, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        TAG(res, iface, enc, NETOPT_CSMA_RETRIES, u8);
    }

    assert(nanocbor_fmt_uint(enc, TAG_FLAG_ARRAY) > 0);
    assert(nanocbor_fmt_array_indefinite(enc) > 0);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_PROMISCUOUSMODE, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_AUTOACK, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_ACK_REQ, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_PRELOADING, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_RAWMODE, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_MAC_NO_SLEEP, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_CSMA, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_AUTOCCA, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_IQ_INVERT, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_SINGLE_RECEIVE, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_CHANNEL_HOP, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_OTAA, enabled);
#ifdef MODULE_GNRC_IPV6
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_IPV6_FORWARDING, enabled);
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_IPV6_SND_RTR_ADV, enabled);
#ifdef MODULE_GNRC_SIXLOWPAN
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_6LO, enabled);
#endif
#if CONFIG_GNRC_IPV6_NIB_6LBR
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_6LO_ABR, enabled);
#endif
#ifdef MODULE_GNRC_SIXLOWPAN_IPHC
    TAG_PRE_DISABLE(res, iface, enc, NETOPT_6LO_IPHC, enabled);
#endif
#endif /* MODULE_GNRC_IPV6 */
    assert(nanocbor_fmt_end_indefinite(enc) > 0);

    TAG(res, iface, enc, NETOPT_MAX_PDU_SIZE, u16);
#ifdef MODULE_GNRC_IPV6
    res = netif_get_opt(iface, NETOPT_MAX_PDU_SIZE, GNRC_NETTYPE_IPV6, &u16, sizeof(u16));
    if (res >= 0) {
        TAG_UINT(enc, TAG_NETOPT_MAX_PDU_SIZE_IPV6, u16);
    }
    res = netif_get_opt(iface, NETOPT_HOP_LIMIT, GNRC_NETTYPE_IPV6, &u8, sizeof(u8));
    if (res >= 0) {
        TAG_UINT(enc, TAG_NETOPT_HOP_LIMIT, u8);
    }
#endif /* MODULE_GNRC_IPV6 */

    TAG(res, iface, enc, NETOPT_SRC_LEN, u16);
    TAG_BOOL(enc, TAG_NETOPT_IS_WIRED, netif_get_opt(iface, NETOPT_IS_WIRED, 0, NULL, 0) > 0);

#if IS_USED(MODULE_IPV6)
    res = netif_get_opt(iface, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
                        sizeof(ipv6_addrs));
    if (res >= 0) {
        unsigned num_ips = res / sizeof(ipv6_addr_t);
        assert(nanocbor_fmt_uint(enc, TAG_NETOPT_IPV6_ADDR) > 0);
        assert(nanocbor_fmt_array(enc, num_ips) > 0);
        // uint8_t ipv6_addrs_flags[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
        // memset(ipv6_addrs_flags, 0, sizeof(ipv6_addrs_flags));

        /* assume it to succeed (otherwise array will stay 0) */
        /* netif_get_opt(iface, NETOPT_IPV6_ADDR_FLAGS, 0, ipv6_addrs_flags,
                      sizeof(ipv6_addrs_flags)); */
        /* yes, the res of NETOPT_IPV6_ADDR is meant to be here ;-) */
        for (unsigned i = 0; i < num_ips; i++) {
            /* 54 is set by iana as IPv6, RFC9164 */
            assert(nanocbor_fmt_tag(enc, TAG_IPV6) > 0);
            assert(sizeof(ipv6_addr_t) == 16);
            assert(nanocbor_put_bstr(enc, (uint8_t *) &ipv6_addrs[i], 16) == NANOCBOR_OK);
            /* TODO _netif_list_ipv6(&ipv6_addrs[i], ipv6_addrs_flags[i]); */
        }
    }
    res = netif_get_opt(iface, NETOPT_IPV6_GROUP, 0, ipv6_addrs,
                        sizeof(ipv6_addrs));
    if (res >= 0) {
        unsigned num_ips = res / sizeof(ipv6_addr_t);
        assert(nanocbor_fmt_uint(enc, TAG_NETOPT_IPV6_GROUP) > 0);
        assert(nanocbor_fmt_array(enc, num_ips) > 0);
        for (unsigned i = 0; i < num_ips; i++) {
            /* 54 is set by iana as IPv6, RFC9164 */
            assert(nanocbor_fmt_tag(enc, TAG_IPV6) > 0);
            assert(nanocbor_fmt_array(enc, 2) > 0);
            /* Prefix, todo */
            assert(nanocbor_fmt_int(enc, 128) > 0);

            assert(sizeof(ipv6_addr_t) == 16);
            assert(nanocbor_put_bstr(enc, (uint8_t *) &ipv6_addrs[i], 16) == NANOCBOR_OK);
        }
    }
#endif

#ifdef MODULE_L2FILTER
//     l2filter_t *filter = NULL;
//     res = netif_get_opt(iface, NETOPT_L2FILTER, 0, &filter, sizeof(filter));
//     if (res > 0) {
// #ifdef MODULE_L2FILTER_WHITELIST
//         printf("\n           White-listed link layer addresses:\n");
// #else
//         printf("\n           Black-listed link layer addresses:\n");
// #endif
//         int count = 0;
//         for (unsigned i = 0; i < CONFIG_L2FILTER_LISTSIZE; i++) {
//             if (filter[i].addr_len > 0) {
//                 char hwaddr_str[filter[i].addr_len * 3];
//                 l2util_addr_to_str(filter[i].addr, filter[i].addr_len,
//                                    hwaddr_str);
//                 printf("            %2i: %s\n", count++, hwaddr_str);
//             }
//         }
//         if (count == 0) {
//             printf("            --- none ---\n");
//         }
//     }
#endif

#ifdef MODULE_NETSTATS_L2
    // _netif_stats(iface, NETSTATS_LAYER2, false);
#endif
#ifdef MODULE_NETSTATS_IPV6
    // _netif_stats(iface, NETSTATS_IPV6, false);
#endif
    assert(nanocbor_fmt_end_indefinite(enc) > 0);
}
