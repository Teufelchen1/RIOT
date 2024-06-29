/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
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
 * @brief       gcoap example
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 *
 * @}
 */

#include <stdio.h>

#include "saul_reg.h"
#include "net/gcoap.h"
#include "fmt.h"
#include "ztimer.h"
#include "mutex.h"
#include <math.h>

#include "gcoap_example.h"

#define MAIN_QUEUE_SIZE     (256)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static mutex_t lock = MUTEX_INIT_LOCKED;

static bool music_host = false;
static char music_host_str[IPV6_ADDR_MAX_STR_LEN+2];

static char * clif_interface_blue = "Blue!";
static char * clif_interface_red = "Red_!";
static char * clif_interface_green = "Gree!";
static char * clif_interface_taken = "Taken";
static size_t clif_interface_len = 5;

//#define MAIN_QUEUE_SIZE (4)
// static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

// static const shell_command_t shell_commands[] = {
//     { "coap", "CoAP example", gcoap_cli_cmd },
//     { NULL, NULL, NULL }
// };

static const char *_devname(saul_reg_t *dev) {
    if (dev->name == NULL) {
        return "(no name)";
    } else {
        return dev->name;
    }
}

/* this function does not check, if the given device is valid */
static void probe(int num, saul_reg_t *dev)
{
    int dim;
    phydat_t res;

    dim = saul_reg_read(dev, &res);
    if (dim <= 0) {
        printf("error: failed to read from device #%i\n", num);
        return;
    }
    /* print results */
    printf("Reading from #%i (%s|", num, _devname(dev));
    saul_class_print(dev->driver->type);
    printf(")\n");
    printf("Values: [%d, %d, %d]\n", res.val[0], res.val[1], res.val[2]);
    //phydat_dump(&res, dim);
}

static int mag(void) {
    // N [594, -612, 673]
    // E [427, -474, 758]
    // S [383, -703, 784]
    // W [422, -315, 831]
    float As[50];
    float Bs[50];
    float Cs[50];

    phydat_t res;

    for (int i = 0; i < 50; ++i)
    {
        saul_reg_read(saul_reg_find_nth(7), &res);
        As[i] = res.val[0];
        Bs[i] = res.val[1];
        Cs[i] = res.val[2];
    }

    float A = res.val[0];
    float B = res.val[1];
    float C = res.val[2];
    
    for (int i = 0; i < 50; ++i)
    {
        A += As[i];
        B += Bs[i];
        C += Cs[i];
    }
    A /= 50+1;
    B /= 50+1;
    C /= 50+1;

    A -= 350;
    B += 640;
    C -= 650;

    A /= 220;
    B /= 200;
    C /= 200;

    //printf("Values: [%d, %d, %d]\n", (int)A, (int)B,(int) C);
    // if (A < 420 && C < 800) {
    //     printf("South [%d, %d, %d]\n", A, B, C);
    // } else if (A > 420 && C > 720) {
    //     printf("East [%d, %d, %d]\n", A, B, C);
    // } else if (A > 450 && C < 800) {
    //     printf("North [%d, %d, %d]\n", A, B, C);
    // } else if (A > 430 && C > 710) {
    //     printf("West [%d, %d, %d]\n", A, B, C);
    // } else {
    //     //printf("Undefined\n");
    //     printf("Values: [%d, %d, %d]\n", A, B, C);
    // }
    // printf("CA: %d, AC: %d, BC: %d, CB:%d, AB: %d, BA: %d\n", 
    //     (int) (atan2(C, A) * 180 / 3.141592653),
    //     (int) (atan2(A, C) * 180 / 3.141592653),
    //     (int) (atan2(B, C) * 180 / 3.141592653),
    //     (int) (atan2(C, B) * 180 / 3.141592653),
    //     (int) (atan2(A, B) * 180 / 3.141592653),
    //     (int) (atan2(B, A) * 180 / 3.141592653)
    //     );
    return (int) (atan2(A, B) * 180 / 3.141592653);
    //printf("Values: [%d, %d, %d]\n", res.val[0], res.val[1], res.val[2]);
}

static void probe_all(void)
{
    saul_reg_t *dev = saul_reg_find_nth(7);
    probe(7, dev);
    dev = saul_reg_find_nth(8);
    probe(8, dev);
    dev = saul_reg_find_nth(11);
    probe(11, dev);

    // dev = saul_reg_find_nth(6);
    // probe(6, dev);
    // dev = saul_reg_find_nth(5);
    // probe(5, dev);
    //  dev = saul_reg_find_nth(4);
    // probe(4, dev);
    // dev = saul_reg_find_nth(3);
    // probe(3, dev);
    //  dev = saul_reg_find_nth(2); // blue
    // probe(2, dev);
    // dev = saul_reg_find_nth(1); // green
    // probe(1, dev);
}

static void _resp_handler(const gcoap_request_memo_t *memo, coap_pkt_t* pdu,
                          const sock_udp_ep_t *remote)
{
    (void)remote;       /* not interested in the source currently */
    if (memo->state == GCOAP_MEMO_TIMEOUT) {
        printf("gcoap: timeout for msg ID %02u\n", coap_get_id(pdu));
        mutex_unlock(&lock);
        return;
    }
    else if (memo->state == GCOAP_MEMO_RESP_TRUNC) {
        /* The right thing to do here would be to look into whether at least
         * the options are complete, then to mentally trim the payload to the
         * next block boundary and pretend it was sent as a Block2 of that
         * size. */
        printf("gcoap: warning, incomplete response; continuing with the truncated payload\n");
    }
    else if (memo->state != GCOAP_MEMO_RESP) {
        printf("gcoap: error in response\n");
        mutex_unlock(&lock);
        return;
    }

    // coap_block1_t block;
    // if (coap_get_block2(pdu, &block) && block.blknum == 0) {
    //     puts("--- blockwise start ---");
    // }

    // char *class_str = (coap_get_code_class(pdu) == COAP_CLASS_SUCCESS)
    //                         ? "Success" : "Error";
    // printf("gcoap: response %s, code %1u.%02u", class_str,
    //                                             coap_get_code_class(pdu),
    //                                             coap_get_code_detail(pdu));
    if (pdu->payload_len) {
        unsigned content_type = coap_get_content_type(pdu);
        if (content_type == COAP_FORMAT_TEXT) {
            // od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
            // od_hex_dump(clif_interface,clif_interface_len, OD_WIDTH_DEFAULT);
            //printf("%.*s\n", pdu->payload_len, (char *)pdu->payload);
            if (pdu->payload_len == clif_interface_len) {
                if (memcmp(clif_interface_blue, pdu->payload, pdu->payload_len) == 0) {
                    music_host = true;
                    phydat_t data;
                    data.val[0] = 255;
                    saul_reg_write(saul_reg_find_nth(2), &data);
                    puts("Found music host blue");
                } else if (memcmp(clif_interface_red, pdu->payload, pdu->payload_len) == 0) {
                    music_host = true;
                    phydat_t data;
                    data.val[0] = 255;
                    saul_reg_write(saul_reg_find_nth(0), &data);
                    puts("Found music host red");
                } else if (memcmp(clif_interface_green, pdu->payload, pdu->payload_len) == 0) {
                    music_host = true;
                    phydat_t data;
                    data.val[0] = 255;
                    saul_reg_write(saul_reg_find_nth(1), &data);
                    puts("Found music host gree");
                } else if (memcmp(clif_interface_taken, pdu->payload, pdu->payload_len) == 0) {
                    puts("Music host taken");
                }
                else {
                    printf("%.*s (%d bytes)\n", pdu->payload_len, (char *)pdu->payload, pdu->payload_len);
                }
            }
        }
        // if (content_type == COAP_FORMAT_TEXT
        //         || content_type == COAP_FORMAT_LINK
        //         || coap_get_code_class(pdu) == COAP_CLASS_CLIENT_FAILURE
        //         || coap_get_code_class(pdu) == COAP_CLASS_SERVER_FAILURE) {
        //     /* Expecting diagnostic payload in failure cases */
        //     printf(", %u bytes\n%.*s\n", pdu->payload_len, pdu->payload_len,
        //                                                   (char *)pdu->payload);
        // }
        // else {
        //     printf(", %u bytes\n", pdu->payload_len);
        //     od_hex_dump(pdu->payload, pdu->payload_len, OD_WIDTH_DEFAULT);
        // }
    }
    else {
        //printf(", empty payload\n");
    }

    /* ask for next block if present */
    // if (coap_get_block2(pdu, &block)) {
    //     if (block.more) {
    //         unsigned msg_type = coap_get_type(pdu);
    //         if (block.blknum == 0 && !strlen(_last_req_path)) {
    //             puts("Path too long; can't complete blockwise");
    //             return;
    //         }

    //         gcoap_req_init(pdu, (uint8_t *)pdu->hdr, CONFIG_GCOAP_PDU_BUF_SIZE,
    //                            COAP_METHOD_GET, _last_req_path);

    //         if (msg_type == COAP_TYPE_ACK) {
    //             coap_hdr_set_type(pdu->hdr, COAP_TYPE_CON);
    //         }
    //         block.blknum++;
    //         coap_opt_add_block2_control(pdu, &block);

    //         int len = coap_opt_finish(pdu, COAP_OPT_FINISH_NONE);
    //         gcoap_req_send((uint8_t *)pdu->hdr, len, remote,
    //                        _resp_handler, memo->context,
    //                        GCOAP_SOCKET_TYPE_UNDEF);
    //     }
    //     else {
    //         // puts("--- blockwise complete ---");
    //     }
    // }
    mutex_unlock(&lock);
}

int main(void)
{
    //probe_all();
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    /* for the thread running the shell */
    //msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    while(1) {
        phydat_t data;
        data.val[0] = 0;
        saul_reg_write(saul_reg_find_nth(0), &data);
        saul_reg_write(saul_reg_find_nth(1), &data);
        saul_reg_write(saul_reg_find_nth(2), &data);
        music_host = false;
        for (int i = 0; i < NEIGHBOUR_LIMIT; ++i)
        {
            memset(&neighbours[i], 0, sizeof(ipv6_addr_t));
        }
        while(!music_host) {
            puts("No music host known.");
            ztimer_sleep(ZTIMER_USEC, 2 * US_PER_SEC);
            int num_neigh = ping_local_multicast();
            if (num_neigh != -1) {
                music_host_str[0] = '[';
                for (int i = 0; i <= num_neigh; ++i)
                {
                    ipv6_addr_to_str(&music_host_str[1], &neighbours[i], sizeof(music_host_str));
                    for (unsigned int k = 0; k < IPV6_ADDR_MAX_STR_LEN+1; ++k)
                    {
                        if (music_host_str[k] == 0) {
                            music_host_str[k] = ']';
                            music_host_str[k+1] = 0;
                            break;
                        }
                    }
                    //printf("Link-local neighbour %d: %s\n", i, music_host_str);
                    gcoap_cli_cmd(COAP_METHOD_GET, music_host_str, "/musichost", NULL, _resp_handler);
                    mutex_lock(&lock);
                    if (music_host) {
                        // we found the music player
                        break;
                    }
                    ztimer_sleep(ZTIMER_USEC, 0.3 * US_PER_SEC);
                }
            } else {
                puts("multicast went wrong.");
                ztimer_sleep(ZTIMER_USEC, 2 * US_PER_SEC);
            }
        }
        printf("Music host: %s\n", music_host_str);
        probe_all();
        //server_init();
        char buffer[] = "123\0\0\0\0\0\0";
        int old_degrees = 0;
        int inactivity_counter = 0;
        while(1){
            inactivity_counter++;
            //puts("gcoap example app");
            ztimer_sleep(ZTIMER_USEC, 0.2 * US_PER_SEC);
            //probe_all();
            int degrees = mag();
            if (degrees < 0) {
                degrees *= -1;
            }
            //printf("Diff: %d\n", (degrees - old_degrees)*(degrees - old_degrees));
            if ((degrees - old_degrees)*(degrees - old_degrees) > 50) {
                inactivity_counter = 0;
                old_degrees = degrees;
                //char * command[] = {"coap", "post", "[fe80:0000:0000:0000:64c3:0c0e:b4b1:e382]", "/speed", buffer};
                fmt_u32_dec(buffer, (int)((degrees/180.0)*360.0));
                //printf("Buffer: %s\n", command[4]);
                int resp = gcoap_cli_cmd(COAP_METHOD_POST, music_host_str, "/speed", buffer, NULL);
                mutex_lock(&resp_lock);
                for (int i = 0; i < 10; ++i)
                {
                    buffer[i]=0;
                }
                if (resp == -1 || RESPONSE != COAP_CLASS_SUCCESS) {
                    // breaks the local endless loop
                    break;
                }
            } else if (inactivity_counter > 35) {
                inactivity_counter = 0;
                // around every 7 seconds
                fmt_u32_dec(buffer, (int)((old_degrees/180.0)*360.0));
                //printf("Buffer: %s\n", command[4]);
                int resp = gcoap_cli_cmd(COAP_METHOD_POST, music_host_str, "/speed", buffer, NULL);
                mutex_lock(&resp_lock);
                for (int i = 0; i < 10; ++i)
                {
                    buffer[i]=0;
                }
                if (resp == -1 || RESPONSE != COAP_CLASS_SUCCESS) {
                    // breaks the local endless loop
                    break;
                }
            }
        }
    }

    /* start shell */
    // puts("All up, running the shell now");
    // char line_buf[SHELL_DEFAULT_BUFSIZE];
    // shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should never be reached */
    return 0;
}
