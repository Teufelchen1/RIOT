/*
 * Copyright (C) 2022 Sören Tempel <tempel@uni-bremen.de>, Bennet Blischke <bennet.blischke@haw-hamburg.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include "shell.h"
#include "msg.h"

#include "thread.h"
#include "fuzzing.h"

#include "net/gnrc/udp.h"
#include "net/sock/util.h"
#include "net/gnrc/pkt.h"
#include "net/cord/config.h"
#include "net/cord/lc.h"
#include "net/ipv6/addr.h"
#include "net/gnrc/nettype.h"
#include "net/gnrc/ipv6/hdr.h"

static uint32_t demux = SERVER_PORT;
static gnrc_nettype_t ntype = GNRC_NETTYPE_UDP;

static cord_lc_rd_t rd;
static sock_udp_ep_t remote;
static char rdbuf[2 * CONFIG_NANOCOAP_URI_MAX] = {0};

//static const shell_command_t shell_commands[] = {
//    { NULL, NULL, NULL },
//};

static void *cord_lc_runner(void *arg)
{
    mutex_t *cordlcmtx = arg;
    mutex_unlock(cordlcmtx);
    char bufpool[1024] = {0};
    int ret = cord_lc_rd_init(&rd, rdbuf, sizeof(rdbuf), &remote);
    if (ret < 0) {
        printf("error initializing RD server %d", ret);
    }
    int retval = cord_lc_raw(&rd, COAP_FORMAT_LINK, CORD_LC_EP, NULL,
                                 bufpool, sizeof(bufpool));
    printf("Lookup result:\n%.*s\n", retval, bufpool);
    return NULL;
}

static void initcordlc(void)
{
    static char cord_lc_thread[THREAD_STACKSIZE_DEFAULT];
    static mutex_t cordlcmtx = MUTEX_INIT_LOCKED;
    kernel_pid_t pid;

    assert(sock_udp_name2ep(&remote, "[" SERVER_ADDR "]") == 0);
    //remote.netif = (uint16_t)gnrc_netif_iter(NULL)->pid;
    remote.family = AF_INET6;
    remote.port = SERVER_PORT;

    pid = thread_create(cord_lc_thread, sizeof(cord_lc_thread), THREAD_PRIORITY_MAIN,
                        0, cord_lc_runner, &cordlcmtx, "cord_lc fuzzing");
    if (pid < 0) {
        errx(EXIT_FAILURE, "thread_create failed: %d\n", pid);
    }

    mutex_lock(&cordlcmtx); /* wait until cord_lc is initialized */
}


void initialize(ipv6_addr_t *addr)
{
    if (ipv6_addr_from_str(addr, SERVER_ADDR) == NULL) {
        errx(EXIT_FAILURE, "ipv6_addr_from_str failed");
    }
    if (fuzzing_init(NULL, 0)) {
        errx(EXIT_FAILURE, "fuzzing_init failed");
    }

    #define MAIN_QUEUE_SIZE     (8)
    //static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    //msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    //puts("CoRE RD lookup client example!\n");
    //char line_buf[SHELL_DEFAULT_BUFSIZE];
    //shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);


    initcordlc();
}

int main(void)
{
    ipv6_addr_t myaddr;
    gnrc_pktsnip_t *ipkt, *upkt, *cpkt;

    initialize(&myaddr);
    return -1;
    if (!(ipkt = gnrc_ipv6_hdr_build(NULL, &myaddr, NULL))) {
        errx(EXIT_FAILURE, "gnrc_ipv6_hdr_build failed");
    }
    if (!(upkt = gnrc_udp_hdr_build(ipkt, SERVER_PORT, COAP_PORT))) {
        errx(EXIT_FAILURE, "gnrc_udp_hdr_build failed");
    }

    if (!(cpkt = gnrc_pktbuf_add(upkt, NULL, 0, GNRC_NETTYPE_UNDEF))) {
        errx(EXIT_FAILURE, "gnrc_pktbuf_add failed");
    }
    if (fuzzing_read_packet(STDIN_FILENO, cpkt)) {
        errx(EXIT_FAILURE, "fuzzing_read_packet failed");
    }

    if (!gnrc_netapi_dispatch_receive(ntype, demux, cpkt)) {
        errx(EXIT_FAILURE, "couldn't find any subscriber");
    }

    return EXIT_SUCCESS;
}
