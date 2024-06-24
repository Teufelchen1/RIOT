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

#include "net/gcoap.h"
#include "ztimer.h"

#include "gcoap_example.h"

//#define MAIN_QUEUE_SIZE (4)
// static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

// static const shell_command_t shell_commands[] = {
//     { "coap", "CoAP example", gcoap_cli_cmd },
//     { NULL, NULL, NULL }
// };

int main(void)
{
    /* for the thread running the shell */
    //msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    //server_init();
    while(1){
        puts("gcoap example app");
        char * command[] = {"coap", "post", "[fe80:0000:0000:0000:64c3:0c0e:b4b1:e382]", "/speed", "180"};
        gcoap_cli_cmd(4, command);
        ztimer_sleep(ZTIMER_USEC, 3 * US_PER_SEC);
    }

    /* start shell */
    // puts("All up, running the shell now");
    // char line_buf[SHELL_DEFAULT_BUFSIZE];
    // shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should never be reached */
    return 0;
}
