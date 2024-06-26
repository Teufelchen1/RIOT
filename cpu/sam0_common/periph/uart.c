/*
 * Copyright (C) 2015 Freie Universität Berlin
 *               2015 FreshTemp, LLC.
 *               2022 SSV Software Systems GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_sam0_common
 * @ingroup     drivers_periph_uart
 * @{
 *
 * @file
 * @brief       Low-level UART driver implementation
 *
 * @author      Thomas Eichinger <thomas.eichinger@fu-berlin.de>
 * @author      Troels Hoffmeyer <troels.d.hoffmeyer@gmail.com>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Dylan Laduranty <dylanladuranty@gmail.com>
 * @author      Benjamin Valentin <benjamin.valentin@ml-pa.com>
 * @author      Juergen Fitschen <me@jue.yt>
 *
 * @}
 */

#include "cpu.h"
#include "pm_layered.h"

#include "periph/uart.h"
#include "periph/gpio.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#if defined (CPU_COMMON_SAML1X) || defined (CPU_COMMON_SAMD5X)
#define UART_HAS_TX_ISR
#endif

/* default to fractional baud rate calculation */
#if !defined(CONFIG_SAM0_UART_BAUD_FRAC) && defined(SERCOM_USART_BAUD_FRAC_BAUD)
/* SAML21 has no fractional baud rate on SERCOM5 */
#if defined(CPU_SAML21)
#define CONFIG_SAM0_UART_BAUD_FRAC  0
#else
#define CONFIG_SAM0_UART_BAUD_FRAC  1
#endif
#endif

/* SAMD20 defines no generic macro */
#ifdef SERCOM_USART_CTRLA_TXPO_PAD0
#undef SERCOM_USART_CTRLA_TXPO
#define SERCOM_USART_CTRLA_TXPO(n) ((n) << SERCOM_USART_CTRLA_TXPO_Pos)
#endif

/**
 * @brief   Allocate memory to store the callback functions & buffers
 */
#ifdef MODULE_PERIPH_UART_NONBLOCKING
#include "tsrb.h"
static tsrb_t uart_tx_rb[UART_NUMOF];
static uint8_t uart_tx_rb_buf[UART_NUMOF][UART_TXBUF_SIZE];
#endif
static uart_isr_ctx_t uart_ctx[UART_NUMOF];

/**
 * @brief   Get the pointer to the base register of the given UART device
 *
 * @param[in] dev       UART device identifier
 *
 * @return              base register address
 */
static inline SercomUsart *dev(uart_t dev)
{
    return uart_config[dev].dev;
}

static inline void _syncbusy(SercomUsart *dev)
{
#ifdef SERCOM_USART_SYNCBUSY_MASK
    while (dev->SYNCBUSY.reg) {}
#else
    while (dev->STATUS.reg & SERCOM_USART_STATUS_SYNCBUSY) {}
#endif
}

static inline void _reset(SercomUsart *dev)
{
    dev->CTRLA.reg = SERCOM_USART_CTRLA_SWRST;
    while (dev->CTRLA.reg & SERCOM_SPI_CTRLA_SWRST) {}

#ifdef SERCOM_USART_SYNCBUSY_MASK
    while (dev->SYNCBUSY.reg & SERCOM_USART_SYNCBUSY_SWRST) {}
#else
    while (dev->STATUS.reg & SERCOM_USART_STATUS_SYNCBUSY) {}
#endif
}

static void _set_baud(uart_t uart, uint32_t baudrate, uint32_t f_src)
{
#if IS_ACTIVE(CONFIG_SAM0_UART_BAUD_FRAC)
    /* Asynchronous Fractional */
    /* BAUD + FP / 8 = f_src / (S * f_baud)       */
    /* BAUD * 8 + FP = (8 * f_src) / (S * f_baud) */
    /* S * (BAUD + 8 * FP) = (8 * f_src) / f_baud */
    uint32_t baud = (f_src * 8) / baudrate;
    dev(uart)->BAUD.FRAC.FP = (baud >> 4) & 0x7; /* baud / 16 */
    dev(uart)->BAUD.FRAC.BAUD = baud >> 7; /* baud / (8 * 16) */
#else
    /* Asynchronous Arithmetic */
    /* BAUD = 2^16     * (2^0 - 2^4 * f_baud / f_src)     */
    /*      = 2^(16-n) * (2^n - 2^(n+4) * f_baud / f_src) */
    /*      = 2^(20-n) * (2^(n-4) - 2^n * f_baud / f_src) */

    /* 2^n * f_baud < 2^32 -> find the next power of 2 */
    uint8_t pow = __builtin_clz(baudrate);

    /* 2^n * f_baud */
    baudrate <<= pow;

    /* (2^(n-4) - 2^n * f_baud / f_src) */
    uint32_t tmp = (1 << (pow - 4)) - baudrate / f_src;
    uint32_t rem = baudrate % f_src;

    uint8_t scale = 20 - pow;
    dev(uart)->BAUD.reg = (tmp << scale) - (rem << scale) / f_src;
#endif
}

void uart_enable_tx(uart_t uart)
{
    /* configure RX pin */
    if (uart_config[uart].tx_pin != GPIO_UNDEF) {
        gpio_init_mux(uart_config[uart].tx_pin, uart_config[uart].mux);
    }
}

void uart_disable_tx(uart_t uart)
{
    /* configure RX pin */
    if (uart_config[uart].tx_pin != GPIO_UNDEF) {
        gpio_init_mux(uart_config[uart].tx_pin, GPIO_MUX_A);
    }
}

static void _configure_pins(uart_t uart)
{
    /* configure RX pin */
    if (uart_config[uart].rx_pin != GPIO_UNDEF) {
        gpio_init(uart_config[uart].rx_pin, GPIO_IN);
        gpio_init_mux(uart_config[uart].rx_pin, uart_config[uart].mux);
    }

    /* configure TX pin */
    if (uart_config[uart].tx_pin != GPIO_UNDEF &&
        !(uart_config[uart].flags & UART_FLAG_TX_ONDEMAND)) {
        gpio_set(uart_config[uart].tx_pin);
        gpio_init(uart_config[uart].tx_pin, GPIO_OUT);
        gpio_init_mux(uart_config[uart].tx_pin, uart_config[uart].mux);
    }

#ifdef MODULE_PERIPH_UART_HW_FC
    /* If RTS/CTS needed, enable them */
    if (uart_config[uart].tx_pad == UART_PAD_TX_0_RTS_2_CTS_3) {
        /* Ensure RTS is defined */
        if (uart_config[uart].rts_pin != GPIO_UNDEF) {
            gpio_init_mux(uart_config[uart].rts_pin, uart_config[uart].mux);
        }
        /* Ensure CTS is defined */
        if (uart_config[uart].cts_pin != GPIO_UNDEF) {
            gpio_init_mux(uart_config[uart].cts_pin, uart_config[uart].mux);
        }
    }
#endif
}

int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg)
{
    if (uart >= UART_NUMOF) {
        return UART_NODEV;
    }

    /* enable peripheral clock */
    sercom_clk_en(dev(uart));

#if IS_ACTIVE(MODULE_PM_LAYERED) && defined(SAM0_UART_PM_BLOCK)
    /* clear previously blocked power modes */
    if (dev(uart)->CTRLA.reg & SERCOM_USART_CTRLA_ENABLE) {
        /* RX IRQ is enabled */
        if (dev(uart)->INTENSET.reg & SERCOM_USART_INTENSET_RXC) {
            pm_unblock(SAM0_UART_PM_BLOCK);
        }
        /* data reg empty IRQ is enabled -> sending data was in progress */
        if (dev(uart)->INTENSET.reg & SERCOM_USART_INTENSET_DRE) {
            pm_unblock(SAM0_UART_PM_BLOCK);
        }
    }
#endif

    /* must disable here first to ensure idempotency */
    dev(uart)->CTRLA.reg = 0;

#ifdef MODULE_PERIPH_UART_NONBLOCKING
    /* set up the TX buffer */
    tsrb_init(&uart_tx_rb[uart], uart_tx_rb_buf[uart], UART_TXBUF_SIZE);
#endif

    /* configure pins */
    _configure_pins(uart);

    /* reset the UART device */
    _reset(dev(uart));

    /* configure clock generator */
    sercom_set_gen(dev(uart), uart_config[uart].gclk_src);

    uint32_t f_src = sam0_gclk_freq(uart_config[uart].gclk_src);

#if IS_ACTIVE(CONFIG_SAM0_UART_BAUD_FRAC)
    uint32_t sampr;
    /* constraint: f_baud ≤ f_src / S */
    if (baudrate * 16 > f_src) {
        /* 8x oversampling */
        sampr = SERCOM_USART_CTRLA_SAMPR(0x3);
        f_src <<= 1;
    } else {
        /* 16x oversampling */
        sampr = SERCOM_USART_CTRLA_SAMPR(0x1);
    }
#endif

    /* set asynchronous mode w/o parity, LSB first, TX and RX pad as specified
     * by the board in the periph_conf.h, x16 sampling and use internal clock */
    dev(uart)->CTRLA.reg = SERCOM_USART_CTRLA_DORD
#if IS_ACTIVE(CONFIG_SAM0_UART_BAUD_FRAC)
    /* enable Asynchronous Fractional mode */
                         | sampr
#endif
                         | SERCOM_USART_CTRLA_TXPO(uart_config[uart].tx_pad)
                         | SERCOM_USART_CTRLA_RXPO(uart_config[uart].rx_pad)
                         | SERCOM_USART_CTRLA_MODE(0x1);

    /* Set run in standby mode if enabled */
    if (uart_config[uart].flags & UART_FLAG_RUN_STANDBY) {
        dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_RUNSTDBY;
    }

    /* calculate and set baudrate */
    _set_baud(uart, baudrate, f_src);

    /* enable transmitter, and configure 8N1 mode */
    if (uart_config[uart].tx_pin != GPIO_UNDEF) {
        dev(uart)->CTRLB.reg = SERCOM_USART_CTRLB_TXEN;
    } else {
        dev(uart)->CTRLB.reg = 0;
    }

    /* enable receiver and RX interrupt if configured */
    if ((rx_cb) && (uart_config[uart].rx_pin != GPIO_UNDEF)) {
        uart_ctx[uart].rx_cb = rx_cb;
        uart_ctx[uart].arg = arg;
#ifdef UART_HAS_TX_ISR
        /* enable RXNE ISR */
        NVIC_EnableIRQ(SERCOM0_2_IRQn + (sercom_id(dev(uart)) * 4));
#else
        /* enable UART ISR */
        NVIC_EnableIRQ(SERCOM0_IRQn + sercom_id(dev(uart)));
#endif /* UART_HAS_TX_ISR */
        dev(uart)->CTRLB.reg |= SERCOM_USART_CTRLB_RXEN;
        dev(uart)->INTENSET.reg = SERCOM_USART_INTENSET_RXC;
#if IS_ACTIVE(MODULE_PM_LAYERED) && defined(SAM0_UART_PM_BLOCK)
        /* block power mode for rx IRQs */
        pm_block(SAM0_UART_PM_BLOCK);
#endif
        /* set wakeup receive from sleep if enabled */
        if (uart_config[uart].flags & UART_FLAG_WAKEUP) {
            dev(uart)->CTRLB.reg |= SERCOM_USART_CTRLB_SFDE;
        }
    }
#ifdef MODULE_PERIPH_UART_NONBLOCKING
#ifndef UART_HAS_TX_ISR
    else {
        /* enable UART ISR */
        NVIC_EnableIRQ(SERCOM0_IRQn + sercom_id(dev(uart)));
    }
#else
    /* enable TXE ISR */
    NVIC_EnableIRQ(SERCOM0_0_IRQn + (sercom_id(dev(uart)) * 4));
#endif
#endif /* MODULE_PERIPH_UART_NONBLOCKING */

    _syncbusy(dev(uart));

    /* and finally enable the device */
    dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;

    return UART_OK;
}

void uart_init_pins(uart_t uart)
{
    _configure_pins(uart);

    uart_poweron(uart);
}

void uart_deinit_pins(uart_t uart)
{
    uart_poweroff(uart);

    /* de-configure RX pin */
    if (uart_config[uart].rx_pin != GPIO_UNDEF) {
        gpio_disable_mux(uart_config[uart].rx_pin);
    }

    /* de-configure TX pin */
    if (uart_config[uart].tx_pin != GPIO_UNDEF) {
        gpio_disable_mux(uart_config[uart].tx_pin);
    }

#ifdef MODULE_PERIPH_UART_HW_FC
    /* If RTS/CTS needed, enable them */
    if (uart_config[uart].tx_pad == UART_PAD_TX_0_RTS_2_CTS_3) {
        /* Ensure RTS is defined */
        if (uart_config[uart].rts_pin != GPIO_UNDEF) {
            gpio_disable_mux(uart_config[uart].rts_pin);
        }
        /* Ensure CTS is defined */
        if (uart_config[uart].cts_pin != GPIO_UNDEF) {
            gpio_disable_mux(uart_config[uart].cts_pin);
        }
    }
#endif
}

gpio_t uart_pin_cts(uart_t uart)
{
#ifdef MODULE_PERIPH_UART_HW_FC
    if (uart_config[uart].tx_pad == UART_PAD_TX_0_RTS_2_CTS_3) {
        return uart_config[uart].cts_pin;
    }
#endif
    (void)uart;
    return GPIO_UNDEF;
}

gpio_t uart_pin_rts(uart_t uart)
{
#ifdef MODULE_PERIPH_UART_HW_FC
    if (uart_config[uart].tx_pad == UART_PAD_TX_0_RTS_2_CTS_3) {
        return uart_config[uart].rts_pin;
    }
#endif
    (void)uart;
    return GPIO_UNDEF;
}

void uart_write(uart_t uart, const uint8_t *data, size_t len)
{
    if (uart_config[uart].tx_pin == GPIO_UNDEF) {
        return;
    }

    if (!(dev(uart)->CTRLA.reg & SERCOM_USART_CTRLA_ENABLE)) {
        return;
    }

#ifdef MODULE_PERIPH_UART_NONBLOCKING
    for (const void* end = data + len; data != end; ++data) {
        if (irq_is_in() || __get_PRIMASK()) {
            /* if ring buffer is full free up a spot */
            if (tsrb_full(&uart_tx_rb[uart])) {
                while (!(dev(uart)->INTFLAG.reg & SERCOM_USART_INTFLAG_DRE)) {}
                dev(uart)->DATA.reg = tsrb_get_one(&uart_tx_rb[uart]);
            }
            tsrb_add_one(&uart_tx_rb[uart], *data);
        }
        else {
            while (tsrb_add_one(&uart_tx_rb[uart], *data) < 0) {}
        }
        /* check and enable DRE IRQs atomically */
#if IS_ACTIVE(MODULE_PM_LAYERED) && defined(SAM0_UART_PM_BLOCK)
        unsigned state = irq_disable();
        /* tsrb_add_one() is blocking the thread. It may happen that
         * the corresponding ISR has turned off DRE IRQs and, thus,
         * unblocked the corresponding power mode. */
        if (!(dev(uart)->INTENSET.reg & SERCOM_USART_INTENSET_DRE)) {
            pm_block(SAM0_UART_PM_BLOCK);
        }
        dev(uart)->INTENSET.reg = SERCOM_USART_INTENSET_DRE;
        irq_restore(state);
#else
        dev(uart)->INTENSET.reg = SERCOM_USART_INTENSET_DRE;
#endif
    }
#else
    for (const void* end = data + len; data != end; ++data) {
        while (!(dev(uart)->INTFLAG.reg & SERCOM_USART_INTFLAG_DRE)) {}
        dev(uart)->DATA.reg = *data;
    }
    while (!(dev(uart)->INTFLAG.reg & SERCOM_USART_INTFLAG_TXC)) {}
#endif
}

void uart_poweron(uart_t uart)
{
    sercom_clk_en(dev(uart));

    /* the enable bit must be read and written atomically */
    unsigned state = irq_disable();
#if IS_ACTIVE(MODULE_PM_LAYERED) && defined(SAM0_UART_PM_BLOCK)
    /* block required power modes */
    if (!(dev(uart)->CTRLA.reg & SERCOM_USART_CTRLA_ENABLE)) {
        /* RX IRQ is enabled */
        if (dev(uart)->INTENSET.reg & SERCOM_USART_INTENSET_RXC) {
            pm_block(SAM0_UART_PM_BLOCK);
        }
        /* data reg empty IRQ is enabled -> sending data was in progress */
        if (dev(uart)->INTENSET.reg & SERCOM_USART_INTENSET_DRE) {
            pm_block(SAM0_UART_PM_BLOCK);
        }
    }
#endif
    dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
    irq_restore(state);

    _syncbusy(dev(uart));
}

void uart_poweroff(uart_t uart)
{
    /* the enable bit must be read and written atomically */
    unsigned state = irq_disable();
#if IS_ACTIVE(MODULE_PM_LAYERED) && defined(SAM0_UART_PM_BLOCK)
    /* clear blocked power modes */
    if (dev(uart)->CTRLA.reg & SERCOM_USART_CTRLA_ENABLE) {
        /* RX IRQ is enabled */
        if (dev(uart)->INTENSET.reg & SERCOM_USART_INTENSET_RXC) {
            pm_unblock(SAM0_UART_PM_BLOCK);
        }
        /* data reg empty IRQ is enabled -> sending data is in progress */
        if (dev(uart)->INTENSET.reg & SERCOM_USART_INTENSET_DRE) {
            pm_unblock(SAM0_UART_PM_BLOCK);
        }
    }
#endif
    dev(uart)->CTRLA.reg &= ~(SERCOM_USART_CTRLA_ENABLE);
    irq_restore(state);

    sercom_clk_dis(dev(uart));
}

#ifdef MODULE_PERIPH_UART_COLLISION
bool uart_collision_detected(uart_t uart)
{
    /* In case of collision, the CTRLB register
     * will be in sync during disabling of TX,
     * then the flag will be set.
     */
    _syncbusy(dev(uart));

    bool collision = dev(uart)->STATUS.reg & SERCOM_USART_STATUS_COLL;
    dev(uart)->STATUS.reg = SERCOM_USART_STATUS_COLL;
    return collision;
}

void uart_collision_detect_enable(uart_t uart)
{
    /* CTRLB is enable protected */
    dev(uart)->CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;
    _syncbusy(dev(uart));

    /* clear stale collision flag */
    dev(uart)->STATUS.reg = SERCOM_USART_STATUS_COLL;

    /* enable collision detection */
    dev(uart)->CTRLB.reg |= SERCOM_USART_CTRLB_COLDEN;

    /* disable RX interrupt */
    dev(uart)->INTENCLR.reg = SERCOM_USART_INTENCLR_RXC;

    /* re-enable UART */
    dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;

    /* wait for config to be applied */
    _syncbusy(dev(uart));
}

static void _drain_rxbuf(SercomUsart *dev)
{
    /* clear readback bytes from receive buffer */
    while (dev->INTFLAG.reg & SERCOM_USART_INTFLAG_RXC) {
        dev->DATA.reg;
    }
}

void uart_collision_detect_disable(uart_t uart)
{
    uint32_t ctrlb = dev(uart)->CTRLB.reg;

    /* re-enable TX after collision */
    ctrlb |= SERCOM_USART_CTRLB_TXEN;

    /* disable collision detection */
    ctrlb &= ~SERCOM_USART_CTRLB_COLDEN;

    /* CTRLB is enable protected */
    dev(uart)->CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;
    _syncbusy(dev(uart));

    dev(uart)->CTRLB.reg = ctrlb;

    /* re-enable UART */
    dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;

    /* wait for config to be applied */
    _syncbusy(dev(uart));

    /* clear bytes from RX buffer */
    _drain_rxbuf(dev(uart));

    /* re-enable RX complete IRQ */
    if (uart_ctx[uart].rx_cb) {
        dev(uart)->INTENSET.reg = SERCOM_USART_INTENSET_RXC;
    }
}
#endif

#ifdef MODULE_PERIPH_UART_MODECFG
int uart_mode(uart_t uart, uart_data_bits_t data_bits, uart_parity_t parity,
              uart_stop_bits_t stop_bits)
{
    if (uart >= UART_NUMOF) {
        return UART_NODEV;
    }

    if (stop_bits != UART_STOP_BITS_1 && stop_bits != UART_STOP_BITS_2) {
        return UART_NOMODE;
    }

    if (parity != UART_PARITY_NONE && parity != UART_PARITY_EVEN &&
            parity != UART_PARITY_ODD) {
        return UART_NOMODE;
    }

    /* Disable UART first to remove write protect */
    dev(uart)->CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;
    _syncbusy(dev(uart));

    uint32_t ctrlb = dev(uart)->CTRLB.reg;

    if (parity == UART_PARITY_NONE) {
        dev(uart)->CTRLA.reg &= ~SERCOM_USART_CTRLA_FORM_Msk;
    }
    else {
        dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_FORM(1);
        if (parity == UART_PARITY_ODD) {
            ctrlb |= SERCOM_USART_CTRLB_PMODE;
        }
        else {
            ctrlb &= ~SERCOM_USART_CTRLB_PMODE;
        }
    }

    if (stop_bits == UART_STOP_BITS_1) {
        ctrlb &= ~SERCOM_USART_CTRLB_SBMODE;
    }
    else {
        ctrlb |= SERCOM_USART_CTRLB_SBMODE;
    }

    dev(uart)->CTRLB.reg = ((ctrlb & ~SERCOM_USART_CTRLB_CHSIZE_Msk) |
                            SERCOM_USART_CTRLB_CHSIZE(data_bits));

    /* Enable UART again */
    dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
    _syncbusy(dev(uart));

    return UART_OK;
}
#endif /* MODULE_PERIPH_UART_MODECFG */

#ifdef MODULE_PERIPH_UART_RXSTART_IRQ
void uart_rxstart_irq_configure(uart_t uart, uart_rxstart_cb_t cb, void *arg)
{
    /* CTRLB is enable-proteced */
    dev(uart)->CTRLA.reg &= ~SERCOM_USART_CTRLA_ENABLE;

    /* set start of frame detection enable */
    dev(uart)->CTRLB.reg |= SERCOM_USART_CTRLB_SFDE;

    uart_ctx[uart].rxs_cb  = cb;
    uart_ctx[uart].rxs_arg = arg;

    /* enable UART again */
    dev(uart)->CTRLA.reg |= SERCOM_USART_CTRLA_ENABLE;
}

void uart_rxstart_irq_enable(uart_t uart)
{
    /* clear stale interrupt flag */
    dev(uart)->INTFLAG.reg  = SERCOM_USART_INTFLAG_RXS;

    /* enable interrupt */
    dev(uart)->INTENSET.reg = SERCOM_USART_INTENSET_RXS;
}

void uart_rxstart_irq_disable(uart_t uart)
{
    dev(uart)->INTENCLR.reg = SERCOM_USART_INTENCLR_RXS;
}
#endif /* MODULE_PERIPH_UART_RXSTART_IRQ */

#ifdef MODULE_PERIPH_UART_NONBLOCKING
static inline void irq_handler_tx(unsigned uartnum)
{
    /* workaround for saml1x */
    int c = tsrb_get_one(&uart_tx_rb[uartnum]);
    if (c >= 0) {
        dev(uartnum)->DATA.reg = c;
    }

    /* disable the interrupt if there are no more bytes to send */
    if (tsrb_empty(&uart_tx_rb[uartnum])) {
        dev(uartnum)->INTENCLR.reg = SERCOM_USART_INTENSET_DRE;
#if IS_ACTIVE(MODULE_PM_LAYERED) && defined(SAM0_UART_PM_BLOCK)
        /* we really should be in IRQ context! */
        assert(irq_is_in());
        pm_unblock(SAM0_UART_PM_BLOCK);
#endif
    }
}
#endif

static inline void irq_handler(unsigned uartnum)
{
    uint32_t status = dev(uartnum)->INTFLAG.reg;
    /* TXC is used by uart_write() */
    dev(uartnum)->INTFLAG.reg = status & ~SERCOM_USART_INTFLAG_TXC;

#if !defined(UART_HAS_TX_ISR) && defined(MODULE_PERIPH_UART_NONBLOCKING)
    if ((status & SERCOM_USART_INTFLAG_DRE) &&
        (dev(uartnum)->INTENSET.reg & SERCOM_USART_INTENSET_DRE)) {
        irq_handler_tx(uartnum);
    }
#endif

#ifdef MODULE_PERIPH_UART_RXSTART_IRQ
    if ((status & SERCOM_USART_INTFLAG_RXS) &&
        (dev(uartnum)->INTENSET.reg & SERCOM_USART_INTENSET_RXS)) {
        uart_ctx[uartnum].rxs_cb(uart_ctx[uartnum].rxs_arg);
    }
#endif

    if (status & SERCOM_USART_INTFLAG_RXC) {
        /* interrupt flag is cleared by reading the data register */
        uart_ctx[uartnum].rx_cb(uart_ctx[uartnum].arg,
                                (uint8_t)(dev(uartnum)->DATA.reg));
    }

    cortexm_isr_end();
}

#ifdef UART_0_ISR
void UART_0_ISR(void)
{
    irq_handler(0);
}
#endif

#ifdef UART_1_ISR
void UART_1_ISR(void)
{
    irq_handler(1);
}
#endif

#ifdef UART_2_ISR
void UART_2_ISR(void)
{
    irq_handler(2);
}
#endif

#ifdef UART_3_ISR
void UART_3_ISR(void)
{
    irq_handler(3);
}
#endif

#ifdef UART_4_ISR
void UART_4_ISR(void)
{
    irq_handler(4);
}
#endif

#ifdef UART_5_ISR
void UART_5_ISR(void)
{
    irq_handler(5);
}
#endif

#ifdef MODULE_PERIPH_UART_NONBLOCKING

#ifdef UART_0_ISR_TX
void UART_0_ISR_TX(void)
{
    irq_handler_tx(0);
}
#endif

#ifdef UART_1_ISR_TX
void UART_1_ISR_TX(void)
{
    irq_handler_tx(1);
}
#endif

#ifdef UART_2_ISR_TX
void UART_2_ISR_TX(void)
{
    irq_handler_tx(2);
}
#endif

#ifdef UART_3_ISR_TX
void UART_3_ISR_TX(void)
{
    irq_handler_tx(3);
}
#endif

#ifdef UART_4_ISR_TX
void UART_4_ISR_TX(void)
{
    irq_handler_tx(4);
}
#endif

#ifdef UART_5_ISR_TX
void UART_5_ISR_TX(void)
{
    irq_handler_tx(5);
}
#endif
#endif /* MODULE_PERIPH_UART_NONBLOCKING */
