/*
 * Copyright (C) 2018 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  Martine Lenders <m.lenders@fu-berlin.de>
 *
 * This implementation oriented itself on the [version by Mike
 * Muuss](http://ftp.arl.army.mil/~mike/ping.html) which was published under
 * public domain. The state-handling and duplicate detection was inspired by the
 * ping version of [inetutils](://www.gnu.org/software/inetutils/), which was
 * published under GPLv3
 */

#ifdef MODULE_GNRC_ICMPV6
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "bitfield.h"
#include "byteorder.h"
#include "msg.h"
#include "net/gnrc.h"
#include "net/gnrc/icmpv6.h"
#include "net/icmpv6.h"
#include "net/ipv6.h"
#include "net/ipv6/addr.h"
#include "net/utils.h"
#include "sched.h"
#include "shell.h"
#include "timex.h"
#include "unaligned.h"
#include "utlist.h"
#include "ztimer.h"
#include "gcoap_example.h"

#ifdef MODULE_LUID
#include "luid.h"
#endif
#ifdef MODULE_GNRC_IPV6_NIB
#include "net/gnrc/ipv6/nib/nc.h"
#endif

#define _SEND_NEXT_PING         (0xEF48)
#define _PING_FINISH            (0xEF49)

#define CKTAB_SIZE              (64U * 8)   /* 64 byte * 8 bit/byte */

#define DEFAULT_COUNT           (3U)
#define DEFAULT_DATALEN         (sizeof(uint32_t))
#define DEFAULT_ID              (0x53)
#define DEFAULT_INTERVAL_USEC   (1U * US_PER_SEC)
#define DEFAULT_TIMEOUT_USEC    (1U * US_PER_SEC)

typedef struct {
    gnrc_netreg_entry_t netreg;
    ztimer_t sched_timer;
    msg_t sched_msg;
    ipv6_addr_t host;
    char *hostname;
    unsigned long num_sent, num_recv, num_rept;
    unsigned long long tsum;
    unsigned tmin, tmax;
    unsigned count;
    size_t datalen;
    BITFIELD(cktab, CKTAB_SIZE);
    uint32_t timeout;
    uint32_t interval;
    gnrc_netif_t *netif;
    uint16_t id;
    uint8_t hoplimit;
} _ping_data_t;


ipv6_addr_t IPV6_UNSPECIFIED = IPV6_ADDR_UNSPECIFIED;
ipv6_addr_t neighbours[NEIGHBOUR_LIMIT];

static void _pinger(_ping_data_t *data);
static int _print_reply(gnrc_pktsnip_t *pkt, int corrupted, uint32_t triptime, void *ctx);

static int _compare_v6(ipv6_addr_t *A, ipv6_addr_t *B) {
    return memcmp((uint8_t *)A, (uint8_t *) B, sizeof(ipv6_addr_t));
}

int ping_local_multicast(void)
{
    _ping_data_t data = {
        .netreg = GNRC_NETREG_ENTRY_INIT_PID(ICMPV6_ECHO_REP,
                                                 thread_getpid()),
        .count = DEFAULT_COUNT,
        .tmin = UINT_MAX,
        .datalen = DEFAULT_DATALEN,
        .timeout = DEFAULT_TIMEOUT_USEC,
        .interval = DEFAULT_INTERVAL_USEC,
        .id = DEFAULT_ID,
    };
    int res = 0;

    ztimer_acquire(ZTIMER_USEC);

    res = netutils_get_ipv6(&data.host, (netif_t **)&data.netif, "ff02::1");
    data.id ^= (ztimer_now(ZTIMER_USEC) & UINT16_MAX);

    gnrc_netreg_register(GNRC_NETTYPE_ICMPV6, &data.netreg);
    _pinger(&data);
    do {
        msg_t msg;

        msg_receive(&msg);
        switch (msg.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV: {
                gnrc_icmpv6_echo_rsp_handle(msg.content.ptr, data.datalen,
                                            _print_reply, &data);
                gnrc_pktbuf_release(msg.content.ptr);
                break;
            }
            case _SEND_NEXT_PING:
                _pinger(&data);
                break;
            case _PING_FINISH:
                goto finish;
            default:
                /* requeue wrong packets */
                msg_send(&msg, thread_getpid());
                break;
        }
    } while (data.num_recv < data.count);
finish:
    ztimer_remove(ZTIMER_USEC, &data.sched_timer);
    gnrc_netreg_unregister(GNRC_NETTYPE_ICMPV6, &data.netreg);
    while (msg_avail() > 0) {
        msg_t msg;

        /* remove all remaining messages (likely caused by duplicates) */
        if ((msg_try_receive(&msg) > 0) &&
            (msg.type == GNRC_NETAPI_MSG_TYPE_RCV) &&
            (((gnrc_pktsnip_t *)msg.content.ptr)->type == GNRC_NETTYPE_ICMPV6)) {
            gnrc_pktbuf_release(msg.content.ptr);
        }
        else {
            /* requeue other packets */
            msg_send(&msg, thread_getpid());
        }
    }
    ztimer_release(ZTIMER_USEC);

    res = -1;
    for (int i = 0; i < NEIGHBOUR_LIMIT; ++i)
    {
        if (_compare_v6(&neighbours[i], &IPV6_UNSPECIFIED) != 0) {
            res++;
        }
    }
    return res;
}

static void _pinger(_ping_data_t *data)
{
    uint32_t timer;
    int res;

    /* schedule next event (next ping or finish) ASAP */
    if ((data->num_sent + 1) < data->count) {
        /* didn't send all pings yet - schedule next in data->interval */
        data->sched_msg.type = _SEND_NEXT_PING;
        timer = data->interval;
    }
    else {
        /* Wait for the last ping to come back.
         * data->timeout: wait for a response in milliseconds.
         * Affects only timeout in absence of any responses,
         * otherwise ping waits for two max RTTs. */
        data->sched_msg.type = _PING_FINISH;
        timer = data->timeout;
        if (data->num_recv) {
            /* approx. 2*tmax, in seconds (2 RTT) */
            timer = (data->tmax / (512UL * 1024UL)) * US_PER_SEC;
            if (timer == 0) {
                timer = 1U * US_PER_SEC;
            }
        }
    }
    ztimer_set_msg(ZTIMER_USEC, &data->sched_timer, timer, &data->sched_msg,
                   thread_getpid());
    bf_unset(data->cktab, (size_t)data->num_sent % CKTAB_SIZE);

    res = gnrc_icmpv6_echo_send(data->netif, &data->host, data->id,
                                data->num_sent++, data->hoplimit, data->datalen);
    switch (-res) {
    case 0:
        break;
    case ENOMEM:
        printf("error: packet buffer full\n");
        break;
    default:
        printf("error: %d\n", res);
        break;
    }
}

static int _print_reply(gnrc_pktsnip_t *pkt, int corrupted, uint32_t triptime, void *ctx)
{
    (void) corrupted;
    (void) triptime;
    _ping_data_t *data = ctx;
    gnrc_pktsnip_t *ipv6 = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_IPV6);
    gnrc_pktsnip_t *icmpv6 = gnrc_pktsnip_search_type(pkt, GNRC_NETTYPE_ICMPV6);

    if (!ipv6 || !icmpv6) {
        return -EINVAL;
    }

    ipv6_hdr_t *ipv6_hdr = ipv6->data;
    icmpv6_echo_t *icmpv6_hdr = icmpv6->data;


    if (icmpv6_hdr->type != ICMPV6_ECHO_REP) {
        return -EINVAL;
    }

    /* not our ping */
    if (byteorder_ntohs(icmpv6_hdr->id) != data->id) {
        return -EINVAL;
    }
    if (!ipv6_addr_is_multicast(&data->host) &&
        !ipv6_addr_equal(&ipv6_hdr->src, &data->host)) {
        return -EINVAL;
    }

    for (int i = 0; i < NEIGHBOUR_LIMIT; ++i)
    {
        // Empty spot
        if (_compare_v6(&neighbours[i], &IPV6_UNSPECIFIED) == 0) {
            memcpy(&neighbours[i], &ipv6_hdr->src, sizeof(ipv6_addr_t));
            return 0;
        }
        // Already in the list 
        if (_compare_v6(&neighbours[i], &ipv6_hdr->src) == 0) {
            return 0;
        }
    }
    // Oh noes, the list is full
    puts("List is full");

    return 0;
}


#endif /* MODULE_GNRC_ICMPV6 */

/** @} */
