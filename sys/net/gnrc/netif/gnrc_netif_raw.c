/*
 * Copyright (C) 2017 Freie Universität Berlin
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
 */

#include "net/gnrc/pktbuf.h"
#include "net/gnrc/netif/hdr.h"
#include "net/gnrc/netif/raw.h"
#include "net/gnrc/udp.h"
#include "net/gnrc/ipv6.h"
//#include <cstdint>

#define ENABLE_DEBUG    1
#include "debug.h"
#include "fmt.h"

#define IP_VERSION_MASK (0xf0U)
#define IP_VERSION4     (0x40U)
#define IP_VERSION6     (0x60U)

static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt);
static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif);

static const gnrc_netif_ops_t raw_ops = {
    .init = gnrc_netif_default_init,
    .send = _send,
    .recv = _recv,
    .get = gnrc_netif_get_from_netdev,
    .set = gnrc_netif_set_from_netdev,
};

int gnrc_netif_raw_create(gnrc_netif_t *netif, char *stack, int stacksize,
                          char priority, char *name, netdev_t *dev)
{
    return gnrc_netif_create(netif, stack, stacksize, priority, name, dev,
                             &raw_ops);
}

static inline uint8_t _get_version(uint8_t *data)
{
    return (data[0] & IP_VERSION_MASK);
}

static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif)
{
    netdev_t *dev = netif->dev;
    int bytes_expected = dev->driver->recv(dev, NULL, 0, NULL);
    gnrc_pktsnip_t *pkt = NULL;

    if (bytes_expected > 0) {
        gnrc_pktsnip_t *hdr;
        //gnrc_pktsnip_t *ip, *udp;
        ipv6_addr_t src = IPV6_ADDR_LOOPBACK;
        ipv6_addr_t dst = IPV6_ADDR_LOOPBACK;
        int nread;
        uint16_t csum = 0;

        pkt = gnrc_pktbuf_add(NULL, NULL, sizeof(ipv6_hdr_t) + sizeof(udp_hdr_t) + bytes_expected, GNRC_NETTYPE_UNDEF);

        ipv6_hdr_t *ip = pkt->data;
        memcpy(&ip->src, &src, sizeof(ipv6_addr_t));
        memcpy(&ip->dst, &dst, sizeof(ipv6_addr_t));
        ip->len = byteorder_htons(sizeof(udp_hdr_t) + bytes_expected);
        ip->hl = 1;
        ip->nh = PROTNUM_UDP;
        ip->v_tc_fl.u8[0] = 0xf0 & 0x60;

        udp_hdr_t *udp = pkt->data + sizeof(ipv6_hdr_t);
        udp->src_port = byteorder_htons(0x1111);
        udp->dst_port = byteorder_htons(0x1633);
        udp->checksum.u16 = 0;
        udp->length = byteorder_htons(sizeof(udp_hdr_t) + bytes_expected);
        //memcpy(pkt->data  + sizeof(ipv6_hdr_t), &udp, sizeof(udp_hdr_t));

        nread = dev->driver->recv(dev, udp + 1, bytes_expected, NULL);
        if (nread <= 1) {   /* we need at least 1 byte to identify IP version */
            DEBUG("gnrc_netif_raw: read error.\n");
            gnrc_pktbuf_release(pkt);
            return NULL;
        }
        //print_bytes_hex(udp+1, bytes_expected);
        csum = inet_csum(csum, (uint8_t*) udp, bytes_expected + sizeof(udp_hdr_t));
        csum = ipv6_hdr_inet_csum(csum, ip, PROTNUM_UDP, bytes_expected + sizeof(udp_hdr_t));
        if (csum == 0xffff) {
            udp->checksum = byteorder_htons(csum);
        }
        else {
            udp->checksum = byteorder_htons(~csum);
        }
        // DEBUG(" csum: %d / %d\n", csum, udp->checksum.u16);
        // DEBUG("coap ptr: %p (hdr: %d)\n", udp + 1, sizeof(udp_hdr_t));
        // DEBUG(" udp ptr: %p\n", udp);
        // DEBUG("  ip ptr: %p\n", ip);


        // memcpy(pkt->data, &ip, sizeof(ipv6_hdr_t));
        // memcpy(pkt->data  + sizeof(ipv6_hdr_t), &udp, sizeof(udp_hdr_t));

        // udp_hdr_t udp;
        // udp.src_port = byteorder_htons(0x1111);
        // udp.dst_port = byteorder_htons(0x1633);
        // udp.checksum.u16 = 0;
        // udp.length = byteorder_htons(bytes_expected);
        // memcpy(pkt->data  + sizeof(ipv6_hdr_t), &udp, sizeof(udp_hdr_t));

        // ipv6_hdr_t ip;
        // memcpy(&ip.src, &src, sizeof(ipv6_addr_t));
        // memcpy(&ip.dst, &dst, sizeof(ipv6_addr_t));
        // ip.len = udp.length;
        // ip.hl = 1;
        // ip.nh = PROTNUM_UDP;
        // ip.v_tc_fl.u8[0] = 0xf0 & 0x60;

        // dev->driver->recv(dev, pkt->data + sizeof(ipv6_hdr_t) + sizeof(udp_hdr_t), bytes_expected, NULL);
        // csum = inet_csum(csum, pkt->data + sizeof(ipv6_hdr_t), bytes_expected + sizeof(udp_hdr_t));
        // csum = ipv6_hdr_inet_csum(csum, &ip, PROTNUM_UDP, bytes_expected);
        // if (csum == 0xffff) {
        //     udp.checksum = byteorder_htons(csum);
        // }
        // else {
        //     udp.checksum = byteorder_htons(~csum);
        // }
        // DEBUG(" csum: %d / %d\n", csum, udp.checksum.u16);


        // memcpy(pkt->data, &ip, sizeof(ipv6_hdr_t));
        // memcpy(pkt->data  + sizeof(ipv6_hdr_t), &udp, sizeof(udp_hdr_t));
        //dev->driver->recv(dev, pkt->data + sizeof(ipv6_hdr_t) + sizeof(udp_hdr_t), bytes_expected, NULL);
        // // udp = gnrc_udp_hdr_build(pkt, 0x1122, 5683);
        // // ip = gnrc_ipv6_hdr_build(udp, &src, &dst);
        // ((ipv6_hdr_t *) ip->data)->hl = 5;
        // ((ipv6_hdr_t *) ip->data)->len = byteorder_htons(48+20);


        //hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
        //gnrc_netif_hdr_set_netif(hdr->data, netif);
        //gnrc_pktsnip_t *rev = gnrc_pktbuf_reverse_snips(ip);
        //gnrc_pktbuf_merge(pkt);

        // hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
        // gnrc_netif_hdr_set_netif(hdr->data, netif);
        // LL_APPEND(pkt, hdr);
        // pkt->type = GNRC_NETTYPE_IPV6;
        // return pkt;
        // gnrc_pktsnip_t *rev = gnrc_pktbuf_reverse_snips(pkt);
        // gnrc_pktbuf_merge(rev);
        // return rev;

        // pkt = gnrc_pktbuf_add(NULL, NULL, bytes_expected, GNRC_NETTYPE_UNDEF);

        // if (!pkt) {
        //     DEBUG("gnrc_netif_raw: cannot allocate pktsnip.\n");
        //     /* drop packet */
        //     dev->driver->recv(dev, NULL, bytes_expected, NULL);
        //     return pkt;
        // }
        // nread = dev->driver->recv(dev, pkt->data, bytes_expected, NULL);
        // if (nread <= 1) {   /* we need at least 1 byte to identify IP version */
        //     DEBUG("gnrc_netif_raw: read error.\n");
        //     gnrc_pktbuf_release(pkt);
        //     return NULL;
        // }

        // udp = gnrc_udp_hdr_build(NULL, 0x1122, 5683);
        // udp = gnrc_pkt_append(pkt, udp);
        // //LL_APPEND(pkt, udp);
        // ip = gnrc_ipv6_hdr_build(NULL, &src, &dst);
        // ip = gnrc_pkt_append(udp, ip);
        // pkt = gnrc_pktbuf_reverse_snips(ip);
        // gnrc_pktbuf_merge(pkt);

        // LL_APPEND(udp, ip);


        hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
        if (!hdr) {
            DEBUG("gnrc_netif_raw: cannot allocate pktsnip.\n");
            gnrc_pktbuf_release(pkt);
            return NULL;
        }
        gnrc_netif_hdr_set_netif(hdr->data, netif);
        //pkt = gnrc_pkt_append(pkt, hdr);

        LL_APPEND(pkt, hdr);
        //return pkt;
#ifdef MODULE_NETSTATS_L2
        netif->stats.rx_count++;
        netif->stats.rx_bytes += nread;
#endif

        if (nread < bytes_expected) {
            /* we've got less then the expected packet size,
             * so free the unused space.*/
            DEBUG("gnrc_netif_raw: reallocating.\n");
            gnrc_pktbuf_realloc_data(pkt, nread);
        }
        switch (_get_version(pkt->data)) {
#ifdef MODULE_GNRC_IPV6
            case IP_VERSION6:
                pkt->type = GNRC_NETTYPE_IPV6;
                break;
#endif
            default:
                /* leave UNDEF */
                break;
        }
    }
    return pkt;
}

static gnrc_pktsnip_t *_skip_pkt_head(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt)
{
    if (gnrc_netif_netdev_legacy_api(netif)) {
        /* we don't need the netif snip: remove it */
        return gnrc_pktbuf_remove_snip(pkt, pkt);
    }
    else {
        /* _tx_done() will free the entire list */
        return pkt->next;
    }
}

static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt)
{
    gnrc_pktbuf_release(pkt);
    return -ENOTSUP;
    int res = -ENOBUFS;

    if (pkt->type == GNRC_NETTYPE_NETIF) {
        pkt = _skip_pkt_head(netif, pkt);
    }

    netdev_t *dev = netif->dev;

#ifdef MODULE_NETSTATS_L2
    netif->stats.tx_unicast_count++;
#endif

    DEBUG("netifraw: asked to send\n");
    res = dev->driver->send(dev, (iolist_t *)pkt);
    if (gnrc_netif_netdev_legacy_api(netif)) {
        /* only for legacy drivers we need to release pkt here */
        DEBUG("netifraw: asked to release\n");
        gnrc_pktbuf_release(pkt);
    }
    return res;
}

/** @} */
