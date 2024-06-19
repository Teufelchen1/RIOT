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
#include "periph/pwm.h"
#include "xtimer.h"
#include "music.h"
#include "500music.h"

void wave(int delay, int step) {
    for (int k = 0; k < 1000; ++k)
    {
        for (int i = 0; i < 256; i += step)
        {
            pwm_set(PWM_DEV(1), 0, i);
            xtimer_usleep(delay);
        }
        for (int i = 250; i > 0; i -= step)
        {
            pwm_set(PWM_DEV(1), 0, i);
            xtimer_usleep(delay);
        }
    }
}

void playOnOff(void) {
    uint8_t music[100] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
    for (int i = 0; i < 100; i++)
    {
        pwm_set(PWM_DEV(1), 0, music[i%100]);
        xtimer_usleep(172);
    }
}

void play285(void) {
    uint8_t music[100] = {128, 135, 143, 151, 159, 167, 174, 182, 189, 196, 202, 208, 214, 220, 225, 230, 235, 239, 242, 246, 248, 251, 252, 253, 254, 255, 254, 253, 252, 251, 248, 246, 242, 239, 235, 230, 225, 220, 214, 208, 202, 196, 189, 182, 174, 167, 159, 151, 143, 135, 128, 120, 112, 104, 96, 88, 81, 73, 66, 59, 53, 47, 41, 35, 30, 25, 20, 16, 13, 9, 7, 4, 3, 2, 1, 1, 1, 2, 3, 4, 7, 9, 13, 16, 20, 25, 30, 35, 41, 47, 53, 59, 66, 73, 81, 88, 96, 104, 112, 120};
    for (int i = 0; i < 100; i++)
    {
        pwm_set(PWM_DEV(1), 0, music[i%100]);
        xtimer_usleep(5);
    }
}

void play570(void) {
    uint8_t music[100] = {128, 143, 159, 174, 189, 202, 214, 225, 235, 242, 248, 252, 254, 254, 252, 248, 242, 235, 225, 214, 202, 189, 174, 159, 143, 128, 112, 96, 81, 66, 53, 41, 30, 20, 13, 7, 3, 1, 1, 3, 7, 13, 20, 30, 41, 53, 66, 81, 96, 112, 127, 143, 159, 174, 189, 202, 214, 225, 235, 242, 248, 252, 254, 254, 252, 248, 242, 235, 225, 214, 202, 189, 174, 159, 143, 128, 112, 96, 81, 66, 53, 41, 30, 20, 13, 7, 3, 1, 1, 3, 7, 13, 20, 30, 41, 53, 66, 81, 96, 112};
    for (int i = 0; i < 100; i++)
    {
        pwm_set(PWM_DEV(1), 0, music[i%100]);
        xtimer_usleep(5);
    }
}

void playfile(void) {
    for (unsigned int i = 0; i < audio_raw_len; i++)
    {
        pwm_set(PWM_DEV(1), 0, audio_raw[i]);
        //xtimer_usleep(86);
        xtimer_spin(xtimer_ticks_from_usec(60));
    }
}

void play500(void) {
    for (unsigned int i = 0; i < hz500_raw_len; i++)
    {
        pwm_set(PWM_DEV(1), 0, hz500_raw[i]);
        //xtimer_usleep(1);
        xtimer_spin(xtimer_ticks_from_usec(47));
    }
}

int main(void)
{
    puts("Hello World!");

    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s CPU.\n", RIOT_CPU);


    uint32_t freq = pwm_init_auto(PWM_DEV(1), PWM_LEFT, 31372, 255);
    //     printf("Set frequency %" PRIu32 ",\n", freq);

     //uint32_t freq = pwm_init(PWM_DEV(1), PWM_LEFT, 31372, 255);
        printf("Set frequency %" PRIu32 ",\n", freq);

    //pwm_set(PWM_DEV(1), 0, 128);

    while(1) {
        //play285();
        //play500();
        //playOnOff();
        //playfile();
         //puts("1");
        //wave(49, 5);
        //xtimer_usleep(1000000);
        // puts("2");
        // wave(30, 5);
        //xtimer_usleep(1000000);
        // puts("3");
        // wave(4, 1);
        // xtimer_usleep(1000000);
        //   puts("4");
        //  wave(40, 5);
        // xtimer_usleep(1000000);
        //  wave(1600);
        // xtimer_usleep(1000000);
        //  wave(3200);
        // xtimer_usleep(1000000);
        // for (int i = 1; i < 150; i += 1) {
        //     wave(i);
        // }
        // for (int i = 150; i > 50; i -= 1) {
        //     wave(i);
        // }
    };
    return 0;
}
