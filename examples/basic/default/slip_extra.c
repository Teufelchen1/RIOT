#include <stdio.h>
#include "thread.h"
#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "msg.h"
#include "fmt.h"

static msg_t _msg_q[4];
static msg_t msg;
static gnrc_netreg_entry_t me_reg;


void slip_recv_init(void)
{
    msg_init_queue(_msg_q, 4);
    //gnrc_pktsnip_t *pkt = NULL;
    me_reg.demux_ctx = 4369;
    me_reg.target.pid = thread_getpid();
    gnrc_netreg_register(GNRC_NETTYPE_UDP, &me_reg);
}

void dump_pkt(gnrc_pktsnip_t *pkt)
{
    gnrc_pktsnip_t *snip = pkt;
    uint8_t lqi = 0;
    if (pkt->next) {
        if (pkt->next->type == GNRC_NETTYPE_NETIF) {
            gnrc_netif_hdr_t *netif_hdr = pkt->next->data;
            lqi = netif_hdr->lqi;
            pkt = gnrc_pktbuf_remove_snip(pkt, pkt->next);
        }
    }
    //uint64_t now_us = ztimer64_now(ZTIMER64_USEC);

    print_str("rftest-rx --- len ");
    print_u32_hex((uint32_t)gnrc_pkt_len(pkt));
    print_str(" lqi ");
    print_byte_hex(lqi);
    print_str(" rx_time ");
    //print_u64_hex(now_us);
    print_str("\n");
    while (snip) {
        for (size_t i = 0; i < snip->size; i++) {
            if (i % 10 == 0) puts("\n");
            print_byte_hex(((uint8_t *)(snip->data))[i]);
            print_str(" ");
        }
        snip = snip->next;
    }
    print_str("\n\n");

    //gnrc_pktbuf_release(pkt);
}

int slip_recv(char *buff) {
    gnrc_pktsnip_t *pkt = NULL;
    msg_t reply;
    reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
    reply.content.value = -ENOTSUP;
    while (1) {
        msg_receive(&msg);
        //puts("Got a packet\n");
        switch (msg.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
                pkt = msg.content.ptr;
                //dump_pkt(pkt);
                memcpy(buff, pkt->data, pkt->size);
                int len = pkt->size;
                //printf("slip_recv: users %d\n\n", pkt->users);
                if (pkt->next) {
                    if (pkt->next->type == GNRC_NETTYPE_NETIF) {
                        pkt = gnrc_pktbuf_remove_snip(pkt, pkt->next);
                    }
                }
                gnrc_pktbuf_release(pkt);
                //puts("yay\n");
                return len;
                //pkt = msg.content.ptr;
                //_handle_incoming_pkt(pkt);
                break;
            case GNRC_NETAPI_MSG_TYPE_SET:
            case GNRC_NETAPI_MSG_TYPE_GET:
                    msg_reply(&msg, &reply);
                    break;
            default:
                break;
        }
    }
    return 0;
}
