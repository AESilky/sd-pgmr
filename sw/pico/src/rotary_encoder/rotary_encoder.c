/**
 * Some content Copyright (c) 2021 pmarques-dev @ github
 *
 * Modifications Copyright 2023 aesilky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
//
// ---- quadrature encoder interface example (from pico-examples)
//
// the PIO program reads phase A/B of a quadrature encoder and increments or
// decrements an internal counter to keep the current absolute step count
// updated. At any point, the main code can query the current count by using
// the quadrature_encoder_*_count functions. The counter is kept in a full
// 32 bit register that just wraps around. Two's complement arithmetic means
// that it can be interpreted as a 32-bit signed or unsigned value, and it will
// work anyway.
//
// As an example, a two wheel robot being controlled at 100Hz, can use two
// state machines to read the two encoders and in the main control loop it can
// simply ask for the current encoder counts to get the absolute step count. It
// can also subtract the values from the last sample to check how many steps
// each wheel as done since the last sample period.
//
// One advantage of this approach is that it requires zero CPU time to keep the
// encoder count updated and because of that it supports very high step rates.
//

// Modifications and additions...
//
// Use interrupts to know when the encoder has moved.
// Add reading of the switch, also using the interrupt to know when it has changed.
//
#include "system_defs.h"
#include "rotary_encoder.h"

#include "board.h"
#include "picohlp/picoutil.h"
#include "cmt/cmt.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"

#include "quadrature_encoder.pio.h"

#include <stdio.h>


#define _PIN_rotary_ENC_AB ROTARY_A_GPIO    // Base pin to connect the A phase of the encoder.

static volatile bool _initialized;
                                            // The B phase must be connected to the next pin
static volatile int16_t _enc_delta;
static volatile int32_t _enc_t_last;
static volatile int32_t _enc_t_prior;
static volatile int32_t _enc_value;

static void _gpio_event_string(char *buf, uint32_t events);

int32_t re_count() {
    return _enc_value;
}

int16_t re_delta() {
    return _enc_delta;
}

int32_t re_tdelta() {
    return _enc_t_last - _enc_t_prior;
}

int32_t re_tlast() {
    return _enc_t_last;
}

void re_turn_irq_handler(uint gpio, uint32_t events) {
    int32_t new_value;
    // note: thanks to two's complement arithmetic delta will always
    // be correct even when new_value wraps around MAXINT / MININT
    new_value = quadrature_encoder_get_count(PIO_ROTARY_BLOCK, PIO_ROTARY_SM);
    _enc_delta = new_value - _enc_value;
    _enc_value = new_value;
    _enc_t_prior = _enc_t_last;
    _enc_t_last = now_ms();

    if (_enc_delta != 0) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_ROTARY_CHG);
        msg.data.value16 = _enc_delta;
        postHIDMsgDiscardable(&msg);
    }
}



void re_module_init() {
    if (_initialized) {
        board_panic("!!! re_module_init: Called more than once !!!");
    }
    // GPIO is initialized in `board.c` with the rest of the board.
    _enc_delta = 0;
    _enc_value = 0;
    _enc_t_prior = now_ms();
    _enc_t_last = _enc_t_prior;
    uint offset = pio_add_program(PIO_ROTARY_BLOCK, &quadrature_encoder_program);
    quadrature_encoder_program_init(PIO_ROTARY_BLOCK, PIO_ROTARY_SM, offset, _PIN_rotary_ENC_AB, 0);

    _initialized = true;
}

