#include <stdio.h>

#include "saul_reg.h"
#include "senml/saul.h"
#include "nanocbor/nanocbor.h"
#include "net/nanocoap.h"

static int _netif_read(netif_t *iface, nanocbor_encoder_t *enc)
{
    int res = 0;

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

    uint8_t hwaddr[GNRC_NETIF_L2ADDR_MAXLEN];
    res = netif_get_opt(iface, NETOPT_ADDRESS_LONG, 0, hwaddr, sizeof(hwaddr));
    if (res >= 0) {
        /* 48 is set by iana as IEEE MAC, RFC9542 */
        assert(nanocbor_fmt_tag(enc, 48) > 0);
        assert(nanocbor_put_bstr(enc, hwaddr, sizeof(hwaddr)) == NANOCBOR_OK);
    }
//     res = netif_get_opt(iface, NETOPT_CHANNEL, 0, &u16, sizeof(u16));
//     if (res >= 0) {
//         printf(" Channel: %" PRIu16 " ", u16);
//     }
//     res = netif_get_opt(iface, NETOPT_CHANNEL_FREQUENCY, 0, &u32, sizeof(u32));
//     if (res >= 0) {
//         printf(" Frequency: %" PRIu32 "Hz ", u32);
//     }
//     res = netif_get_opt(iface, NETOPT_CHANNEL_PAGE, 0, &u16, sizeof(u16));
//     if (res >= 0) {
//         printf(" Page: %" PRIu16 " ", u16);
//     }
//     res = netif_get_opt(iface, NETOPT_NID, 0, &u16, sizeof(u16));
//     if (res >= 0) {
//         printf(" NID: 0x%" PRIx16 " ", u16);
//     }
//     res = netif_get_opt(iface, NETOPT_RSSI, 0, &i16, sizeof(i16));
//     if (res >= 0) {
//         printf(" RSSI: %d ", i16);
//     }

//     netopt_enable_t enabled;
//     res = netif_get_opt(iface, NETOPT_LINK, 0, &enabled, sizeof(enabled));
//     if (res >= 0) {
//         printf(" Link: %s ", (enabled == NETOPT_ENABLE) ? "up" : "down" );
//     }

//     line_thresh = _newline(0U, line_thresh);
//     res = netif_get_opt(iface, NETOPT_ADDRESS_LONG, 0, hwaddr, sizeof(hwaddr));
//     if (res >= 0) {
//         char hwaddr_str[res * 3];
//         printf("Long HWaddr: ");
//         printf("%s ", l2util_addr_to_str(hwaddr, res, hwaddr_str));
//         line_thresh++;
//     }
//     line_thresh = _newline(0U, line_thresh);
//     res = netif_get_opt(iface, NETOPT_TX_POWER, 0, &i16, sizeof(i16));
//     if (res >= 0) {
//         printf(" TX-Power: %" PRIi16 "dBm ", i16);
//     }
//     res = netif_get_opt(iface, NETOPT_STATE, 0, &state, sizeof(state));
//     if (res >= 0) {
//         printf(" State: %s ", _netopt_state_str[state]);
//         line_thresh++;
//     }
//     res = netif_get_opt(iface, NETOPT_RETRANS, 0, &u8, sizeof(u8));
//     if (res >= 0) {
//         printf(" max. Retrans.: %u ", (unsigned)u8);
//         line_thresh++;
//     }
//     res = netif_get_opt(iface, NETOPT_CSMA_RETRIES, 0, &u8, sizeof(u8));
//     if (res >= 0) {
//         enabled = NETOPT_DISABLE;
//         res = netif_get_opt(iface, NETOPT_CSMA, 0, &enabled, sizeof(enabled));
//         if ((res >= 0) && (enabled == NETOPT_ENABLE)) {
//             printf(" CSMA Retries: %u ", (unsigned)u8);
//         }
//         line_thresh++;
//     }
//     /* XXX divide options and flags by at least two spaces! */
//     line_thresh = _newline(0U, line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_PROMISCUOUSMODE, "PROMISC  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_AUTOACK, "AUTOACK  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_ACK_REQ, "ACK_REQ  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_PRELOADING, "PRELOAD  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_RAWMODE, "RAWMODE  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_MAC_NO_SLEEP, "MAC_NO_SLEEP  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_CSMA, "CSMA  ",
//                                    line_thresh);
//     line_thresh += _LINE_THRESHOLD + 1; /* enforce linebreak after this option */
//     line_thresh = _netif_list_flag(iface, NETOPT_AUTOCCA, "AUTOCCA  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_IQ_INVERT, "IQ_INVERT  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_SINGLE_RECEIVE, "RX_SINGLE  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_CHANNEL_HOP, "CHAN_HOP  ",
//                                    line_thresh);
//     line_thresh = _netif_list_flag(iface, NETOPT_OTAA, "OTAA  ",
//                                    line_thresh);
//     /* XXX divide options and flags by at least two spaces! */
//     res = netif_get_opt(iface, NETOPT_MAX_PDU_SIZE, 0, &u16, sizeof(u16));
//     if (res > 0) {
//         printf("L2-PDU:%" PRIu16 "  ", u16);
//         line_thresh++;
//     }
//     res = netif_get_opt(iface, NETOPT_SRC_LEN, 0, &u16, sizeof(u16));
//     /* XXX divide options and flags by at least two spaces before this line! */
//     if (res >= 0) {
//         printf("Source address length: %" PRIu16, u16);
//         line_thresh++;
//     }
//     line_thresh = _newline(0U, line_thresh);

    /* 302 as wired/wireless, self choosen */
    assert(nanocbor_fmt_tag(enc, 302) > 0);
    bool is_wired = netif_get_opt(iface, NETOPT_IS_WIRED, 0, NULL, 0) > 0;
    assert(nanocbor_fmt_bool(enc,is_wired ) > 0);

#ifdef MODULE_IPV6
    ipv6_addr_t ipv6_addrs[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
    ipv6_addr_t ipv6_groups[GNRC_NETIF_IPV6_GROUPS_NUMOF];

    res = netif_get_opt(iface, NETOPT_IPV6_ADDR, 0, ipv6_addrs,
                        sizeof(ipv6_addrs));
    if (res >= 0) {
        uint8_t ipv6_addrs_flags[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
        memset(ipv6_addrs_flags, 0, sizeof(ipv6_addrs_flags));

        /* assume it to succeed (otherwise array will stay 0) */
        /* netif_get_opt(iface, NETOPT_IPV6_ADDR_FLAGS, 0, ipv6_addrs_flags,
                      sizeof(ipv6_addrs_flags)); */
        /* yes, the res of NETOPT_IPV6_ADDR is meant to be here ;-) */
        for (unsigned i = 0; i < (res / sizeof(ipv6_addr_t)); i++) {
            /* 54 is set by iana as IPv6, RFC9164 */
            assert(nanocbor_fmt_tag(enc, 54) > 0);
            assert(sizeof(ipv6_addr_t) == 16);
            assert(nanocbor_put_bstr(enc, (uint8_t *) &ipv6_addrs[i], 16) == NANOCBOR_OK);
            /* _netif_list_ipv6(&ipv6_addrs[i], ipv6_addrs_flags[i]); */
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

    assert(nanocbor_fmt_end_indefinite(enc) > 0);
    return 0;
}

static int _netif_read_name(const char *name, size_t len, nanocbor_encoder_t *enc)
{
    netif_t *netif = netif_get_by_name_buffer(name, len);
    return _netif_read(netif, enc);
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
        _netif_read(netif, enc);
        last = netif;
    }

    assert(nanocbor_fmt_end_indefinite(enc) > 0);
}

static int _convert_errorno_to_coap_code(int num)
{
    switch (num) {
    case -ECANCELED:
    case -EIO:
        return COAP_CODE_INTERNAL_SERVER_ERROR;
    case -ENODEV:
        return COAP_CODE_404;
    case -ENOTSUP:
        return COAP_CODE_METHOD_NOT_ALLOWED;
    default:
        if (num < 0) {
            return COAP_CODE_INTERNAL_SERVER_ERROR;
        } 
        return COAP_CODE_CONTENT;
    }
}

static ssize_t _coap_bad_request(coap_pkt_t *pkt, uint8_t *buf, size_t len)
{
    return coap_reply_simple(pkt, COAP_CODE_BAD_REQUEST, buf, len,
                    COAP_FORMAT_NONE, NULL, 0);
}

static ssize_t _coap_unprocessable_entity(coap_pkt_t *pkt, uint8_t *buf, size_t len)
{
    return coap_reply_simple(pkt, COAP_CODE_UNPROCESSABLE_ENTITY, buf, len,
                    COAP_FORMAT_NONE, NULL, 0);
}

static ssize_t _coap_internal_server_error(coap_pkt_t *pkt, uint8_t *buf, size_t len)
{
    return coap_reply_simple(pkt, COAP_CODE_INTERNAL_SERVER_ERROR, buf, len,
                    COAP_FORMAT_NONE, NULL, 0);
}

static ssize_t _coap_not_found(coap_pkt_t *pkt, uint8_t *buf, size_t len)
{
    return coap_reply_simple(pkt, COAP_CODE_404, buf, len,
                    COAP_FORMAT_NONE, NULL, 0);
}

ssize_t _gnrc_netif_handler(coap_pkt_t *pkt, uint8_t *buf, size_t len,
                                  coap_request_ctx_t *context)
{
    (void) context;
    uint8_t buffer[500];

    nanocbor_encoder_t enc;
    nanocbor_encoder_init(&enc, buffer, sizeof(buffer));

    nanocbor_value_t decoder;
    nanocbor_value_t array;

    nanocbor_decoder_init(&decoder, pkt->payload, pkt->payload_len);
    switch (coap_method2flag(coap_get_code_raw(pkt))) {
    default:
    case COAP_GET:
        /* Are we looking for one or more specific interface(s)? */
        if(nanocbor_enter_array(&decoder, &array) == NANOCBOR_OK) {
            /* Start response array, we don't know how many interfaces it will contain */
            if (nanocbor_fmt_array_indefinite(&enc) <= 0) {
                return _coap_bad_request(pkt, buf, len);
            }

            char *name_buf = NULL;
            size_t name_len = 0;
            /* for each interface... */
            while (!nanocbor_at_end(&array)) {
                /* which name should we look up? */
                if (nanocbor_get_tstr(&array,(const uint8_t **) &name_buf, &name_len) < 0) {
                    return _coap_unprocessable_entity(pkt, buf, len);
                }
                /* look up the interface with that name */
                /* the functions takes care of the response encoding */
                int ret = _netif_read_name(name_buf, name_len, &enc);
                if (ret < 0) {
                    int coap_code = _convert_errorno_to_coap_code(ret);
                    return coap_reply_simple(pkt, coap_code, buf, len,
                        COAP_FORMAT_NONE, NULL, 0);
                }
            }
            if (nanocbor_fmt_end_indefinite(&enc) <= 0) {
                return _coap_internal_server_error(pkt, buf, len);
            }
        } else {
            _list(&enc);
        }
        break;
    case COAP_PATCH:
        if (nanocbor_enter_array(&decoder, &array) == NANOCBOR_OK) {
            uint32_t tag = 0;

            if (nanocbor_get_tag(&array, &tag) != NANOCBOR_OK) {
                return _coap_bad_request(pkt, buf, len);
            }

            if (tag != 20) {
                return _coap_bad_request(pkt, buf, len);
            }

            char *name_buf = NULL;
            size_t name_len = 0;

            if (nanocbor_get_tstr(&array, (const uint8_t **) &name_buf, &name_len) < 0) {
                return _coap_bad_request(pkt, buf, len);
            }
            netif_t *iface = netif_get_by_name_buffer(name_buf, name_len);

            if (iface == NULL) {
                return _coap_not_found(pkt, buf, len);
            }

            if (nanocbor_get_tag(&array, &tag) != NANOCBOR_OK) {
                return _coap_bad_request(pkt, buf, len);
            }
            if (tag == 303) {
                bool netopt_change = false;
                if (nanocbor_get_bool(&array, &netopt_change) != NANOCBOR_OK) {
                    return _coap_internal_server_error(pkt, buf, len);
                }
                netopt_enable_t en_or_dis = netopt_change ? NETOPT_ENABLE : NETOPT_DISABLE;
#if IS_USED(MODULE_LWIP_NETIF) /* lwIP sets netif state, not link state */
                if (netif_set_opt(iface, NETOPT_ACTIVE, 0, &en_or_dis, sizeof(en_or_dis)) < 0) {
                    return _coap_internal_server_error(pkt, buf, len);
                }
#else
                if (netif_set_opt(iface, NETOPT_LINK, 0, &en_or_dis, sizeof(en_or_dis)) < 0) {
                    return _coap_internal_server_error(pkt, buf, len);
                }
#endif
            } 
            if (tag == 54) {
#ifdef MODULE_GNRC_IPV6
                    ipv6_addr_t *ipv6_buffer;
                    size_t ipv6_buffer_len;
                    //printf("Next is: %d\n", nanocbor_get_type(&array))
                    if (nanocbor_get_bstr(&array, (const uint8_t **) &ipv6_buffer, &ipv6_buffer_len) != NANOCBOR_OK) {
                        //printf("Can not get ipaddr\n");
                        return _coap_unprocessable_entity(pkt, buf, len);
                    }
                    if (ipv6_buffer_len != 16) {
                        return _coap_bad_request(pkt, buf, len);
                    }

                    if (ipv6_addr_is_multicast(ipv6_buffer)) {
                        if (netif_set_opt(iface, NETOPT_IPV6_GROUP_LEAVE, 0, ipv6_buffer,
                                          ipv6_buffer_len) < 0) {
                            printf("error: unable to leave IPv6 multicast group\n");
                            return 1;
                        }
                    }
                    else {
                        if (netif_set_opt(iface, NETOPT_IPV6_ADDR_REMOVE, 0, ipv6_buffer,
                                          ipv6_buffer_len) < 0) {
                            printf("error: unable to remove IPv6 address\n");
                            return 1;
                        }
                    }
#endif
            }
        }
        break;
    case COAP_PUT:
    case COAP_POST:
        if(nanocbor_enter_array(&decoder, &array) == NANOCBOR_OK) {
            uint32_t tag = 0;

            if (nanocbor_get_tag(&array, &tag) != NANOCBOR_OK) {
                return _coap_bad_request(pkt, buf, len);
            }

            if (tag != 20) {
                return _coap_bad_request(pkt, buf, len);
            }

            char *name_buf = NULL;
            size_t name_len = 0;

            if (nanocbor_get_tstr(&array, (const uint8_t **) &name_buf, &name_len) < 0) {
                return _coap_bad_request(pkt, buf, len);
            }
            netif_t *iface = netif_get_by_name_buffer(name_buf, name_len);

            if (iface == NULL) {
                return _coap_not_found(pkt, buf, len);
            }

            
            if (nanocbor_get_tag(&array, &tag) != NANOCBOR_OK) {
                return _coap_bad_request(pkt, buf, len);
            }
            if (tag == 54) {
                nanocbor_value_t inner_array;
                if(nanocbor_enter_array(&array, &inner_array) == NANOCBOR_OK) {
                    ipv6_addr_t *ipv6_buffer;
                    size_t ipv6_buffer_len;
                    //printf("Next is: %d\n", nanocbor_get_type(&array))
                    if (nanocbor_get_bstr(&inner_array, (const uint8_t **) &ipv6_buffer, &ipv6_buffer_len) != NANOCBOR_OK) {
                        //printf("Can not get ipaddr\n");
                        return _coap_unprocessable_entity(pkt, buf, len);
                    }
                    if (ipv6_buffer_len != 16) {
                        return _coap_bad_request(pkt, buf, len);
                    }

                    uint8_t prefix = 128;
                    if (nanocbor_get_uint8(&inner_array, &prefix) < 0) {
                        return _coap_bad_request(pkt, buf, len);
                    }
                    uint16_t flags = GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID | (prefix << 8);
                    if (netif_set_opt(iface,
                        NETOPT_IPV6_ADDR, flags, ipv6_buffer, ipv6_buffer_len) < 0) {
                        return _coap_internal_server_error(pkt, buf, len);
                    }
                } else {
                    return _coap_bad_request(pkt, buf, len);
                }
            }
        } else {
            return _coap_bad_request(pkt, buf, len);
        }
        break;
       
    }
    return coap_reply_simple(pkt, COAP_CODE_205, buf, len,
        COAP_FORMAT_CBOR, buffer, nanocbor_encoded_len(&enc));
}

NANOCOAP_RESOURCE(netif_cbor) { \
    .path = "/jelly/netif", \
    .methods = COAP_GET | COAP_POST | COAP_PUT | COAP_PATCH,\
    .handler = _gnrc_netif_handler, \
    .context = NULL \
};


// static void _netif_list(netif_t *iface)
// {
// #ifdef MODULE_IPV6
//     ipv6_addr_t ipv6_addrs[CONFIG_GNRC_NETIF_IPV6_ADDRS_NUMOF];
//     ipv6_addr_t ipv6_groups[GNRC_NETIF_IPV6_GROUPS_NUMOF];
// #endif
// #if IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORA)
//     res = netif_get_opt(iface, NETOPT_BANDWIDTH, 0, &u8, sizeof(u8));
//     if (res >= 0) {
//         printf(" BW: %skHz ", _netopt_bandwidth_str[u8]);
//     }
//     res = netif_get_opt(iface, NETOPT_SPREADING_FACTOR, 0, &u8, sizeof(u8));
//     if (res >= 0) {
//         printf(" SF: %u ", u8);
//     }
//     res = netif_get_opt(iface, NETOPT_CODING_RATE, 0, &u8, sizeof(u8));
//     if (res >= 0) {
//         printf(" CR: %s ", _netopt_coding_rate_str[u8]);
//     }
// #endif /* MODULE_SHELL_CMD_GNRC_NETIF_LORA */
// #ifdef MODULE_NETDEV_IEEE802154
//     res = netif_get_opt(iface, NETOPT_IEEE802154_PHY, 0, &u8, sizeof(u8));
//     if (res >= 0) {
//         printf(" PHY: %s ", _netopt_ieee802154_phy_str[u8]);
//         switch (u8) {

// #ifdef MODULE_NETDEV_IEEE802154_OQPSK
//         case IEEE802154_PHY_OQPSK:
//             printf("\n          ");
//             res = netif_get_opt(iface, NETOPT_OQPSK_RATE, 0, &u8, sizeof(u8));
//             if (res >= 0 && u8) {
//                 printf(" high data rate: %d ", u8);
//             }

//             break;

// #endif /* MODULE_NETDEV_IEEE802154_OQPSK */
// #ifdef MODULE_NETDEV_IEEE802154_MR_OQPSK
//         case IEEE802154_PHY_MR_OQPSK:
//             printf("\n          ");
//             res = netif_get_opt(iface, NETOPT_MR_OQPSK_CHIPS, 0, &u16, sizeof(u16));
//             if (res >= 0) {
//                 printf(" chip rate: %u ", u16);
//             }
//             res = netif_get_opt(iface, NETOPT_MR_OQPSK_RATE, 0, &u8, sizeof(u8));
//             if (res >= 0) {
//                 printf(" rate mode: %d ", u8);
//             }

//             break;

// #endif /* MODULE_NETDEV_IEEE802154_MR_OQPSK */
// #ifdef MODULE_NETDEV_IEEE802154_MR_OFDM
//         case IEEE802154_PHY_MR_OFDM:
//             printf("\n          ");
//             res = netif_get_opt(iface, NETOPT_MR_OFDM_OPTION, 0, &u8, sizeof(u8));
//             if (res >= 0) {
//                 printf(" Option: %u ", u8);
//             }
//             res = netif_get_opt(iface, NETOPT_MR_OFDM_MCS, 0, &u8, sizeof(u8));
//             if (res >= 0) {
//                 printf(" MCS: %u (%s) ", u8, _netopt_ofdm_mcs_str[u8]);
//             }

//             break;
// #endif /* MODULE_NETDEV_IEEE802154_MR_OFDM */
// #ifdef MODULE_NETDEV_IEEE802154_MR_FSK
//         case IEEE802154_PHY_MR_FSK:
//             printf("\n          ");
//             res = netif_get_opt(iface, NETOPT_MR_FSK_MODULATION_INDEX, 0, &u8, sizeof(u8));
//             if (res >= 0) {
//                 hwaddr[0] = 64; /* convenient temp var */
//                 frac_short(&u8, hwaddr);
//                 if (hwaddr[0] == 1) {
//                     printf(" modulation index: %u ", u8);
//                 }
//                 else {
//                     printf(" modulation index: %u/%u ", u8, hwaddr[0]);
//                 }
//             }
//             res = netif_get_opt(iface, NETOPT_MR_FSK_MODULATION_ORDER, 0, &u8, sizeof(u8));
//             if (res >= 0) {
//                 printf(" %u-FSK ", u8);
//             }
//             res = netif_get_opt(iface, NETOPT_MR_FSK_SRATE, 0, &u16, sizeof(u16));
//             if (res >= 0) {
//                 printf(" symbol rate: %u kHz ", u16);
//             }
//             res = netif_get_opt(iface, NETOPT_MR_FSK_FEC, 0, &u8, sizeof(u8));
//             if (res >= 0) {
//                 printf(" FEC: %s ", _netopt_fec_str[u8]);
//             }
//             res = netif_get_opt(iface, NETOPT_CHANNEL_SPACING, 0, &u16, sizeof(u16));
//             if (res >= 0) {
//                 printf(" BW: %ukHz ", u16);
//             }

//             break;
// #endif /* MODULE_NETDEV_IEEE802154_MR_FSK */
//         }
//     }
// #endif /* MODULE_NETDEV_IEEE802154 */
// #if IS_USED(MODULE_LWIP_NETIF) /* only supported on lwIP for now */
//     res = netif_get_opt(iface, NETOPT_ACTIVE, 0, &enabled, sizeof(enabled));
//     if (res >= 0) {
//         printf(" State: %s ", (enabled == NETOPT_ENABLE) ? "up" : "down" );
//     }
// #endif /* MODULE_LWIP_NETIF */
// #if IS_USED(MODULE_SHELL_CMD_GNRC_NETIF_LORAWAN)
//     res = netif_get_opt(iface, NETOPT_DEMOD_MARGIN, 0, &u8, sizeof(u8));
//     if (res >= 0) {
//         printf(" Demod margin.: %u ", (unsigned)u8);
//         line_thresh++;
//     }
//     res = netif_get_opt(iface, NETOPT_NUM_GATEWAYS, 0, &u8, sizeof(u8));
//     if (res >= 0) {
//         printf(" Num gateways.: %u ", (unsigned)u8);
//         line_thresh++;
//     }
// #endif
    
// #ifdef MODULE_GNRC_IPV6
//     res = netif_get_opt(iface, NETOPT_MAX_PDU_SIZE, GNRC_NETTYPE_IPV6, &u16, sizeof(u16));
//     if (res > 0) {
//         printf("MTU:%" PRIu16 "  ", u16);
//         line_thresh++;
//     }
//     res = netif_get_opt(iface, NETOPT_HOP_LIMIT, 0, &u8, sizeof(u8));
//     if (res > 0) {
//         printf("HL:%u  ", u8);
//         line_thresh++;
//     }
//     line_thresh = _netif_list_flag(iface, NETOPT_IPV6_FORWARDING, "RTR  ",
//                                    line_thresh);
// #ifndef MODULE_GNRC_SIXLOWPAN_IPHC
//     line_thresh += _LINE_THRESHOLD + 1; /* enforce linebreak after this option */
// #endif
//     line_thresh = _netif_list_flag(iface, NETOPT_IPV6_SND_RTR_ADV, "RTR_ADV  ",
//                                    line_thresh);
// #ifdef MODULE_GNRC_SIXLOWPAN
//     line_thresh = _netif_list_flag(iface, NETOPT_6LO, "6LO  ", line_thresh);
// #endif
// #if CONFIG_GNRC_IPV6_NIB_6LBR
//     line_thresh = _netif_list_flag(iface, NETOPT_6LO_ABR, "ABR  ", line_thresh);
// #endif
// #ifdef MODULE_GNRC_SIXLOWPAN_IPHC
//     line_thresh += _LINE_THRESHOLD + 1; /* enforce linebreak after this option */
//     line_thresh = _netif_list_flag(iface, NETOPT_6LO_IPHC, "IPHC  ",
//                                    line_thresh);
// #endif
// #endif
// #ifdef MODULE_L2FILTER
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
// #endif

// #ifdef MODULE_NETSTATS_L2
//     puts("");
//     _netif_stats(iface, NETSTATS_LAYER2, false);
// #endif
// #ifdef MODULE_NETSTATS_IPV6
//     _netif_stats(iface, NETSTATS_IPV6, false);
// #endif
//     puts("");
// }
