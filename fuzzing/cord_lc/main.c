/*
 * Copyright (C) 2018 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       CoRE Resource Directory lookup (cord_lc) example
 *
 * @author      Aiman Ismail <muhammadaimanbin.ismail@haw-hamburg.de>
 *
 * @}
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include "thread.h"
#include "fuzzing.h"

#include "net/cord/config.h"
#include "net/cord/lc.h"
#include "net/gnrc/netif.h"
#include "net/gcoap.h"
#include "net/sock/util.h"
#include "net/ipv6/addr.h"
#include "net/gnrc/nettype.h"
#include "net/gnrc/ipv6/hdr.h"
#include "net/gnrc/pkt.h"
#include "net/gnrc/udp.h"

#include "msg.h"


static cord_lc_rd_t rd;
static sock_udp_ep_t remote;
static char rdbuf[2 * CONFIG_NANOCOAP_URI_MAX] = {0};

static int make_sock_ep(sock_udp_ep_t *ep, const char *addr)
{
    ep->port = 0;
    if (sock_udp_name2ep(ep, addr) < 0) {
        return -1;
    }
    /* if netif not specified in addr */
    if ((ep->netif == SOCK_ADDR_ANY_NETIF) && (gnrc_netif_numof() == 1)) {
        /* assign the single interface found in gnrc_netif_numof() */
        ep->netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
    }
    ep->family  = AF_INET6;
    if (ep->port == 0) {
        ep->port = COAP_PORT;
    }
    return 0;
}

/*
static void _print_lookup_result(struct cord_lc_result *res) {
    printf("Found resource/endpoint\n");
    printf("Target: %.*s\n", res->link.target_len, res->link.target);
    for (unsigned i = 0; i < res->link.attrs_len; i++) {
        clif_attr_t *p = &(res->link.attrs[i]);
        printf("'%.*s': '%.*s'\n", p->key_len, p->key, p->value_len, p->value);
    }
}
*/

static void *cord_lc_runner(void *arg)
{
    mutex_t *cordlcmtx = arg;
    // char bufpool[1024] = {0};

    int ret = make_sock_ep(&remote, "[" SERVER_ADDR "]:" SERVER_PORT);
    if (ret < 0) {
        printf("error: unable to parse address\n");
        return NULL;
    }
    puts("Performing lookup now, this may take a short while...");
    mutex_unlock(cordlcmtx);
    ret = cord_lc_rd_init(&rd, rdbuf, sizeof(rdbuf), &remote);
    if (ret < 0) {
        printf("error initializing RD server %d", ret);
        return NULL;
    }

    printf("Lookup interface: %s\n", rd.ep_lookif);
    /*
    cord_lc_filter_t filters = {
        .array = NULL,
        .len = 0,
        .next = NULL,
    };

    int retval = 0;
    cord_lc_ep_t endpoint;
    clif_attr_t attrs[5];
    endpoint.attrs = attrs;
    endpoint.max_attrs = ARRAY_SIZE(attrs);
    retval = cord_lc_ep(&rd, &endpoint, &filters, bufpool, sizeof(bufpool));
    if (retval < 0) {
        printf("Error during lookup %d\n", retval);
        return -1;
    }
    _print_lookup_result(&endpoint);
    */
    ztimer_sleep(ZTIMER_SEC, 0);
    return NULL;
}


void initialize(ipv6_addr_t *nodeaddr, ipv6_addr_t *rdaddr)
{
    if (ipv6_addr_from_str(nodeaddr, "2001:db8::8") == NULL) {
        errx(EXIT_FAILURE, "ipv6_addr_from_str failed");
    }
    if (ipv6_addr_from_str(rdaddr, "2001:db8::1") == NULL) {
        errx(EXIT_FAILURE, "ipv6_addr_from_str failed");
    }

    if (fuzzing_init(NULL, 0)) {
        errx(EXIT_FAILURE, "fuzzing_init failed");
    }

    static char cord_lc_thread[THREAD_STACKSIZE_DEFAULT];
    static mutex_t cordlcmtx = MUTEX_INIT_LOCKED;

    kernel_pid_t pid = thread_create(cord_lc_thread, sizeof(cord_lc_thread), THREAD_PRIORITY_MAIN,
                        0, cord_lc_runner, &cordlcmtx, "cord_lc fuzzing");
    printf("cord_lc thread pid: %d\n", pid);
    if (pid < 0) {
        errx(EXIT_FAILURE, "thread_create failed: %d\n", pid);
    }

    mutex_lock(&cordlcmtx); /* wait until cord_lc is initialized */
}

static uint32_t demux = COAP_PORT;
static gnrc_nettype_t ntype = GNRC_NETTYPE_UDP;

int main(void)
{
    ipv6_addr_t nodeaddr;
    ipv6_addr_t rdaddr;
    gnrc_pktsnip_t *ipkt, *upkt, *cpkt;

    initialize(&nodeaddr, &rdaddr);
    ztimer_sleep(ZTIMER_SEC, 1);
    
    if (!(ipkt = gnrc_ipv6_hdr_build(NULL, &rdaddr, &nodeaddr))) {
        errx(EXIT_FAILURE, "gnrc_ipv6_hdr_build failed");
    }
    if (!(upkt = gnrc_udp_hdr_build(ipkt, 4223, COAP_PORT))) {
        errx(EXIT_FAILURE, "gnrc_udp_hdr_build failed");
    }

    if (!(cpkt = gnrc_pktbuf_add(upkt, NULL, 0, GNRC_NETTYPE_UNDEF))) {
        errx(EXIT_FAILURE, "gnrc_pktbuf_add failed");
    }
    if (fuzzing_read_packet(STDIN_FILENO, cpkt)) {
        errx(EXIT_FAILURE, "fuzzing_read_packet failed");
    }
    /*
    printf("Packetraw:\n");
    for (size_t i = 0; i < cpkt->size; ++i)
    {
        printf("%x ", ((uint8_t *) cpkt->data)[i]);
    }
    puts("");
    */
    if (!gnrc_netapi_dispatch_receive(ntype, demux, cpkt)) {
        errx(EXIT_FAILURE, "couldn't find any subscriber");
    }
    while(1){    printf("done\n"); }
    ztimer_sleep(ZTIMER_SEC, 2);
    return EXIT_SUCCESS;
}
