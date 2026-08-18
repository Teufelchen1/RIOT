#include <stdio.h>

#include "nanocbor/nanocbor.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netif/ipv6.h"
#include "net/ipv6/addr.h"
#include "net/unicoap.h"

static int _netif_list(netif_t *iface, nanocbor_encoder_t *enc);

static int _get_netif_from_cbor(nanocbor_value_t *decoder, netif_t **iface)
{
    uint32_t tag = 0;

    if (nanocbor_get_tag(decoder, &tag) != NANOCBOR_OK) {
        return -EBADMSG;
    }

    /* netifname is 20, self choosen */
    if (tag != 20) {
        return -EINVAL;
    }

    char *name_buf = NULL;
    size_t name_len = 0;

    if (nanocbor_get_tstr(decoder, (const uint8_t **) &name_buf, &name_len) < 0) {
        return -EBADMSG;
    }
    *iface = netif_get_by_name_buffer(name_buf, name_len);

    if (*iface == NULL) {
        return -ENODEV;
    }

    return 1;
}

static int _tag303(nanocbor_value_t *decoder, netif_t *netif, unicoap_method_t method)
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

static int _tag54(nanocbor_value_t *decoder, netif_t *netif, unicoap_method_t method)
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
    if (method == UNICOAP_METHOD_POST) {
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

static void _list(nanocbor_encoder_t *enc)
{
    assert(nanocbor_fmt_array_indefinite(enc) > 0);

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
        _netif_list(netif, enc);
        last = netif;
    }

    assert(nanocbor_fmt_end_indefinite(enc) > 0);
}

int _gnrc_netif_handler(unicoap_message_t* message, const unicoap_aux_t* aux,
    unicoap_request_context_t* ctx, void* arg)
{
    (void) arg;
    (void) aux;

    uint8_t *payload = unicoap_message_payload_get(message);
    size_t payload_len = unicoap_message_payload_get_size(message);

    uint8_t buffer[256];

    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buffer, sizeof(buffer));

    nanocbor_value_t decoder;
    nanocbor_decoder_init(&decoder, payload, payload_len);

    unicoap_method_t method = unicoap_request_get_method(message);
    if (method == UNICOAP_METHOD_GET) {
        if (nanocbor_get_type(&decoder) == NANOCBOR_TYPE_TAG) {
            netif_t *netif = NULL;
            int ret = _get_netif_from_cbor(&decoder, &netif);
            if (ret < 0) {
                return unicoap_response_status_from_errno(ret);
            }

            assert(nanocbor_fmt_array(&enc, 1) > 0);
            _netif_list(netif, &enc);
        } else {
            _list(&enc);
        }

        UNICOAP_OPTIONS_ALLOC(options, 2);

        if (unicoap_options_set_content_format(&options, UNICOAP_FORMAT_CBOR) < 0) {
            return UNICOAP_STATUS_INTERNAL_SERVER_ERROR;
        }

        unicoap_response_init_with_options(message, UNICOAP_STATUS_CONTENT, buffer, nanocbor_encoded_len(&enc), &options);
        return unicoap_send_response(message, ctx);
    }

    nanocbor_value_t array;
    if (nanocbor_enter_array(&decoder, &array) != NANOCBOR_OK) {
        return UNICOAP_STATUS_BAD_REQUEST;
    }

    netif_t *netif = NULL;
    int ret = _get_netif_from_cbor(&array, &netif);
    if (ret < 0) {
        return unicoap_response_status_from_errno(ret);
    }

    uint32_t tag = 0;
    if (nanocbor_get_tag(&array, &tag) != NANOCBOR_OK) {
        return UNICOAP_STATUS_BAD_REQUEST;
    }

    switch (tag) {
    case 303:
        if ((ret = _tag303(&array, netif, method)) < 0) {
            return unicoap_response_status_from_errno(ret);
        }
        break;
    case 54:
        if ((ret = _tag54(&array, netif, method)) < 0) {
            return unicoap_response_status_from_errno(ret);
        }
        break;
    default:
        return UNICOAP_STATUS_UNPROCESSABLE_ENTITY;
    }

    return UNICOAP_STATUS_CHANGED;
}

UNICOAP_RESOURCE(netif_cbor) {
  .path = UNICOAP_PATH("jelly", "netif"),
  .handler = _gnrc_netif_handler,
  .methods = UNICOAP_METHODS(UNICOAP_METHOD_GET, UNICOAP_METHOD_PUT, UNICOAP_METHOD_POST, UNICOAP_METHOD_PATCH),
  .protocols = UNICOAP_PROTOCOLS(UNICOAP_PROTO_SLIPMUX),
};

#define TAG_NETOPT_CHANNEL_NUM 304
#define TAG_NETOPT_CHANNEL_TYP u16
#define TAG_NETOPT_CHANNEL_FREQUENCY_NUM 305
#define TAG_NETOPT_CHANNEL_FREQUENCY_TYP u32
#define TAG_NETOPT_CHANNEL_PAGE_NUM 306
#define TAG_NETOPT_CHANNEL_PAGE_TYP u16
#define TAG_NETOPT_NID_NUM 307
#define TAG_NETOPT_NID_TYP u16
#define TAG_NETOPT_RSSI_NUM 308
#define TAG_NETOPT_RSSI_TYP i16
#define TAG_NETOPT_OQPSK_RATE_NUM 315
#define TAG_NETOPT_OQPSK_RATE_TYP u8
#define TAG_NETOPT_MR_OQPSK_CHIPS_NUM 316
#define TAG_NETOPT_MR_OQPSK_CHIPS_TYP u16
#define TAG_NETOPT_MR_OQPSK_RATE_NUM 317
#define TAG_NETOPT_MR_OQPSK_RATE_TYP u8
#define TAG_NETOPT_MR_OFDM_OPTION_NUM 318
#define TAG_NETOPT_MR_OFDM_OPTION_TYP u8
#define TAG_NETOPT_MR_OFDM_MCS_NUM 319
#define TAG_NETOPT_MR_OFDM_MCS_TYP u8
#define TAG_NETOPT_MR_FSK_MODULATION_INDEX_NUM 320
#define TAG_NETOPT_MR_FSK_MODULATION_INDEX_TYP u8
#define TAG_NETOPT_MR_FSK_MODULATION_ORDER_NUM 321
#define TAG_NETOPT_MR_FSK_MODULATION_ORDER_TYP u8
#define TAG_NETOPT_MR_FSK_SRATE_NUM 322
#define TAG_NETOPT_MR_FSK_SRATE_TYP u16
#define TAG_NETOPT_MR_FSK_FEC_NUM 323
#define TAG_NETOPT_MR_FSK_FEC_TYP u8
#define TAG_NETOPT_CHANNEL_SPACING_NUM 324
#define TAG_NETOPT_CHANNEL_SPACING_TYP u16

#define TAG_u32(enc, num, data) TAG_UINT(enc, num, data)
#define TAG_u16(enc, num, data) TAG_UINT(enc, num, data)
#define TAG_i16(enc, num, data) TAG_INT(enc, num, data)

#define CAT(A,B) CAT_(A,B)
#define CAT_(A,B) A##B

#define TAG_NUM(type) TAG_ ## type ##_NUM
#define TAG_TYP(type) TAG_ ## type ##_TYP

#define TAG(res, iface, enc, netopt, data) do {\
    res = netif_get_opt(iface, netopt, 0, &TAG_TYP(netopt), sizeof(TAG_TYP(netopt)));\
    if (res >= 0) { \
        CAT(TAG_, TAG_TYP(netopt))(enc, TAG_NUM(netopt), data); \
    }\
} while(0)

#define TAG_UINT(encoder, number, payload) do {     \
    assert(nanocbor_fmt_tag(encoder, number) > 0);  \
    assert(nanocbor_fmt_uint(encoder, payload) > 0);\
} while(0)

#define TAG_INT(encoder, number, payload) do {     \
    assert(nanocbor_fmt_tag(encoder, number) > 0); \
    assert(nanocbor_fmt_int(encoder, payload) > 0);\
} while(0)

static int _netif_list(netif_t *iface, nanocbor_encoder_t *enc)
{
#ifdef MODULE_IPV6
    ipv6_addr_t ipv6_addrs[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
    ipv6_addr_t ipv6_groups[GNRC_NETIF_IPV6_GROUPS_NUMOF];
#endif
    uint8_t hwaddr[GNRC_NETIF_L2ADDR_MAXLEN];
    uint32_t u32;
    uint16_t u16;
    int16_t i16;
    uint8_t u8;
    int res;

    if (iface == NULL) {
        return -ENODEV;
    }

    assert(nanocbor_fmt_array_indefinite(enc) > 0);

    char name[CONFIG_NETIF_NAMELENMAX];

    netif_get_name(iface, name);

    /* 20 as name, self choosen */
    assert(nanocbor_fmt_tag(enc, 20) > 0);
    /* safe: netif_get_name promises to be null terminated */
    assert(nanocbor_put_tstr(enc, name) == NANOCBOR_OK);

    // res = netif_get_opt(iface, NETOPT_ADDRESS, 0, hwaddr, sizeof(hwaddr));
    // if (res >= 0) {
    //     char hwaddr_str[res * 3];
    //     printf(" HWaddr: %s ",
    //            l2util_addr_to_str(hwaddr, res, hwaddr_str));
    // }
    // res = netif_get_opt(iface, NETOPT_CHANNEL, 0, &u16, sizeof(u16));
    TAG(res, iface, enc, NETOPT_CHANNEL, u16);   
    TAG(res, iface, enc, NETOPT_CHANNEL_FREQUENCY, u32);
    TAG(res, iface, enc, NETOPT_CHANNEL_PAGE, u16);
    TAG(res, iface, enc, NETOPT_NID, u16);
    TAG(res, iface, enc, NETOPT_RSSI, i16);

#if IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORA)
    // res = netif_get_opt(iface, NETOPT_BANDWIDTH, 0, &u8, sizeof(u8));
    // if (res >= 0) {
    //     printf(" BW: %skHz ", _netopt_bandwidth_str[u8]);
    // }
    // res = netif_get_opt(iface, NETOPT_SPREADING_FACTOR, 0, &u8, sizeof(u8));
    // if (res >= 0) {
    //     printf(" SF: %u ", u8);
    // }
    // res = netif_get_opt(iface, NETOPT_CODING_RATE, 0, &u8, sizeof(u8));
    // if (res >= 0) {
    //     printf(" CR: %s ", _netopt_coding_rate_str[u8]);
    // }
#endif /* MODULE_SHELL_CMD_GNRC_NETIF_LORA */
#ifdef MODULE_NETDEV_IEEE802154
    res = netif_get_opt(iface, NETOPT_IEEE802154_PHY, 0, &u8, sizeof(u8));
    if (res >= 0) {
        assert(nanocbor_fmt_tag(enc, 325) > 0);
        assert(nanocbor_fmt_array_indefinite(enc) > 0);

        assert(nanocbor_fmt_tag(enc, 314) > 0);
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
    netopt_enable_t enabled;
    res = netif_get_opt(iface, NETOPT_LINK, 0, &enabled, sizeof(enabled));
    if (res >= 0) {
        // 303
        assert(nanocbor_fmt_tag(enc, 309) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
#if IS_USED(MODULE_LWIP_NETIF) /* only supported on lwIP for now */
    // res = netif_get_opt(iface, NETOPT_ACTIVE, 0, &enabled, sizeof(enabled));
    // if (res >= 0) {
    //     printf(" State: %s ", (enabled == NETOPT_ENABLE) ? "up" : "down" );
    // }
#endif /* MODULE_LWIP_NETIF */
    res = netif_get_opt(iface, NETOPT_ADDRESS_LONG, 0, hwaddr, sizeof(hwaddr));
    if (res >= 0) {
        /* 48 is set by iana as IEEE MAC, RFC9542 */
        assert(nanocbor_fmt_tag(enc, 48) > 0);
        assert(nanocbor_put_bstr(enc, hwaddr, sizeof(hwaddr)) == NANOCBOR_OK);
    }
    res = netif_get_opt(iface, NETOPT_TX_POWER, 0, &i16, sizeof(i16));
    if (res >= 0) {
        assert(nanocbor_fmt_tag(enc, 310) > 0);
        assert(nanocbor_fmt_int(enc, i16) > 0);
    }
    netopt_state_t state;
    res = netif_get_opt(iface, NETOPT_STATE, 0, &state, sizeof(state));
    if (res >= 0) {
        assert(nanocbor_fmt_tag(enc, 311) > 0);
        assert(nanocbor_fmt_int(enc, state) > 0);
    }
    res = netif_get_opt(iface, NETOPT_RETRANS, 0, &u8, sizeof(u8));
    if (res >= 0) {
        assert(nanocbor_fmt_tag(enc, 312) > 0);
        assert(nanocbor_fmt_uint(enc, u8) > 0);
    }
    res = netif_get_opt(iface, NETOPT_CSMA_RETRIES, 0, &u8, sizeof(u8));
    if (res >= 0) {
        enabled = NETOPT_DISABLE;
        res = netif_get_opt(iface, NETOPT_CSMA, 0, &enabled, sizeof(enabled));
        if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
            assert(nanocbor_fmt_tag(enc, 313) > 0);
            assert(nanocbor_fmt_uint(enc, u8) > 0);
        }
    }
#if IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORAWAN)
    // res = netif_get_opt(iface, NETOPT_DEMOD_MARGIN, 0, &u8, sizeof(u8));
    // if (res >= 0) {
    //     printf(" Demod margin.: %u ", (unsigned)u8);
    //     line_thresh++;
    // }
    // res = netif_get_opt(iface, NETOPT_NUM_GATEWAYS, 0, &u8, sizeof(u8));
    // if (res >= 0) {
    //     printf(" Num gateways.: %u ", (unsigned)u8);
    //     line_thresh++;
    // }
#endif
    /* XXX divide options and flags by at least two spaces! */
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_PROMISCUOUSMODE, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 326) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_AUTOACK, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 327) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_ACK_REQ, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 328) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_PRELOADING, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 329) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_RAWMODE, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 330) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_MAC_NO_SLEEP, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 331) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_CSMA, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 332) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_AUTOCCA, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 333) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_IQ_INVERT, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 334) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_SINGLE_RECEIVE, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 335) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_CHANNEL_HOP, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 336) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_OTAA, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 337) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    /* XXX divide options and flags by at least two spaces! */
    res = netif_get_opt(iface, NETOPT_MAX_PDU_SIZE, 0, &u16, sizeof(u16));
    if (res >= 0) {
        assert(nanocbor_fmt_tag(enc, 343) > 0);
        assert(nanocbor_fmt_uint(enc, u16) > 0);
    }
#ifdef MODULE_GNRC_IPV6
    res = netif_get_opt(iface, NETOPT_MAX_PDU_SIZE, GNRC_NETTYPE_IPV6, &u16, sizeof(u16));
    if (res >= 0) {
        assert(nanocbor_fmt_tag(enc, 344) > 0);
        assert(nanocbor_fmt_uint(enc, u16) > 0);
    }
    res = netif_get_opt(iface, NETOPT_HOP_LIMIT, GNRC_NETTYPE_IPV6, &u8, sizeof(u8));
    if (res >= 0) {
        assert(nanocbor_fmt_tag(enc, 345) > 0);
        assert(nanocbor_fmt_uint(enc, u8) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_IPV6_FORWARDING, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 338) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_IPV6_SND_RTR_ADV, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 339) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
#ifdef MODULE_GNRC_SIXLOWPAN
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_6LO, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 340) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
#endif
#if CONFIG_GNRC_IPV6_NIB_6LBR
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_6LO_ABR, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 341) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
#endif
#ifdef MODULE_GNRC_SIXLOWPAN_IPHC
    enabled = NETOPT_DISABLE;
    res = netif_get_opt(iface, NETOPT_6LO_IPHC, 0, &enabled, sizeof(enabled));
    if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
        assert(nanocbor_fmt_tag(enc, 342) > 0);
        assert(nanocbor_fmt_bool(enc, (enabled == NETOPT_ENABLE)) > 0);
    }
#endif
#endif /* MODULE_GNRC_IPV6 */
    // res = netif_get_opt(iface, NETOPT_SRC_LEN, 0, &u16, sizeof(u16));
    // /* XXX divide options and flags by at least two spaces before this line! */
    // if (res >= 0) {
    //     printf("Source address length: %" PRIu16, u16);
    //     line_thresh++;
    // }
    /* 302 as wired/wireless, self choosen */
    assert(nanocbor_fmt_tag(enc, 302) > 0);
    bool is_wired = netif_get_opt(iface, NETOPT_IS_WIRED, 0, NULL, 0) > 0;
    assert(nanocbor_fmt_bool(enc,is_wired ) > 0);
#ifdef MODULE_IPV6
    res = netif_get_opt(iface, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
                        sizeof(ipv6_addrs));
    if (res >= 0) {
        // uint8_t ipv6_addrs_flags[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
        // memset(ipv6_addrs_flags, 0, sizeof(ipv6_addrs_flags));

        /* assume it to succeed (otherwise array will stay 0) */
        /* netif_get_opt(iface, NETOPT_IPV6_ADDR_FLAGS, 0, ipv6_addrs_flags,
                      sizeof(ipv6_addrs_flags)); */
        /* yes, the res of NETOPT_IPV6_ADDR is meant to be here ;-) */
        for (unsigned i = 0; i < (res / sizeof(ipv6_addr_t)); i++) {
            /* 54 is set by iana as IPv6, RFC9164 */
            assert(nanocbor_fmt_tag(enc, 54) > 0);
            assert(sizeof(ipv6_addr_t) == 16);
            assert(nanocbor_put_bstr(enc, (uint8_t *) &ipv6_addrs[i], 16) == NANOCBOR_OK);
            /* TODO _netif_list_ipv6(&ipv6_addrs[i], ipv6_addrs_flags[i]); */
        }
    }
    res = netif_get_opt(iface, NETOPT_IPV6_GROUP, 0, ipv6_groups,
                        sizeof(ipv6_groups));
    if (res >= 0) {
        for (unsigned i = 0; i < (res / sizeof(ipv6_addr_t)); i++) {
            /* 54 is set by iana as IPv6, RFC9164 */
            assert(nanocbor_fmt_tag(enc, 54) > 0);
            assert(nanocbor_fmt_array(enc, 2) > 0);
            /* Prefix, todo */
            assert(nanocbor_fmt_int(enc, 128) > 0);

            assert(sizeof(ipv6_addr_t) == 16);
            assert(nanocbor_put_bstr(enc, (uint8_t *) &ipv6_groups[i], 16) == NANOCBOR_OK);
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
    // puts("");
    // _netif_stats(iface, NETSTATS_LAYER2, false);
#endif
#ifdef MODULE_NETSTATS_IPV6
    // _netif_stats(iface, NETSTATS_IPV6, false);
#endif
    assert(nanocbor_fmt_end_indefinite(enc) > 0);
    return 0;
}
