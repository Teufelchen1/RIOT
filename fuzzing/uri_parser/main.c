/*
 * Copyright (C) 2022 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <err.h>

#include "uri_parser.h"
#include "fuzzing.h"

__AFL_FUZZ_INIT()

#pragma clang optimize off

int main(void)
{
    __AFL_INIT();
    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) {
        int len = __AFL_FUZZ_TESTCASE_LEN;
        uri_parser_result_t uri_res;
        uri_parser_process(&uri_res, (char *)buf, len);
    }

    exit(EXIT_SUCCESS);
    return 0;
}
