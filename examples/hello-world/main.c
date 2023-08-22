/*
 * Copyright (C) 2014 Freie Universität Berlin
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
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include "net/gcoap/forward_proxy.h"
#include "net/gcoap.h"
#include "net/sock/udp.h"
#include "net/sock/util.h"

int main(void)
{
    puts("Hello World!");

    char linebuffer[512];
    int pos = 0;
    unsigned char buf[512];
    sock_udp_ep_t client;
    coap_pkt_t pkt;
    if(sock_udp_str2ep(&client, "[::1]:1234") != 0) {
        puts("Failed to setup socket");
        return -1;
    }
    int res = gcoap_req_init(&pkt, buf, sizeof(buf), 0, "/");
    if (res < 0) {
        puts("Failed to init packet");
        return -1;
    }

    puts("Enter coap forwarding uri: ");
    while(1) {
        int c = getchar();
        if (c == '\n') {
            linebuffer[pos] = '\0';
            break;
        }
        linebuffer[pos++] = (char) c;
    }
    puts(linebuffer);

    if(coap_opt_add_proxy_uri(&pkt, (const char *) linebuffer) < 0) {
        puts("Failed to add proxy uri");
        return -1;
    }
    gcoap_forward_proxy_request_process(&pkt, &client);

    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);

    return 0;
}
