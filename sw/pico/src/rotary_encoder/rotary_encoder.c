/**
 * Some content Copyright (c) 2021 pmarques-dev @ github
 *
 * Modifications Copyright 2023-25 aesilky
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
#include "picoutil.h"
#include "cmt.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
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
static volatile int32_t _enc_t_delta;
static volatile int32_t _enc_value;
static volatile int32_t _enc_velocity;

static void _gpio_event_string(char *buf, uint32_t events);


// max_step_rate is used to lower the clock of the state machine to save power
// if the application doesn't require a very high sampling rate. Passing zero
// will set the clock to the maximum, which gives a max step rate of around
// 8.9 Msteps/sec at 125MHz
static inline void _quadrature_encoder_program_init(PIO pio, uint sm, uint offset, uint pin, int max_step_rate) {
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 2, false);
    gpio_pull_up(pin);
    gpio_pull_up(pin + 1);
    pio_sm_config c = quadrature_encoder_program_get_default_config(offset);
    sm_config_set_in_pins(&c, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&c, pin); // for JMP
    // shift to left, autopull disabled
    sm_config_set_in_shift(&c, false, false, 32);
    // don't join FIFO's
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_NONE);
    // passing "0" as the sample frequency,
    if (max_step_rate == 0) {
        sm_config_set_clkdiv(&c, 1.0);
    }
    else {
     // one state machine loop takes at most 14 cycles
        float div = (float)clock_get_hz(clk_sys) / (14 * max_step_rate);
        sm_config_set_clkdiv(&c, div);
    }
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}
// When requesting the current count we may have to wait a few cycles (average
// ~11 sysclk cycles) for the state machine to reply. If we are reading multiple
// encoders, we may request them all in one go and then fetch them all, thus
// avoiding doing the wait multiple times. If we are reading just one encoder,
// we can use the "get_count" function to request and wait
static inline void _quadrature_encoder_request_count(PIO pio, uint sm) {
    pio->txf[sm] = 1;
}
static inline int32_t _quadrature_encoder_fetch_count(PIO pio, uint sm) {
    bool mt = true;
    for (int i = 0; mt && i < 1000; i++) {
        mt = pio_sm_is_rx_fifo_empty(pio, sm);
    }
    return (mt ? -1 : pio->rxf[sm]);
}
static inline int32_t _quadrature_encoder_get_count(PIO pio, uint sm) {
    _quadrature_encoder_request_count(pio, sm);
    return _quadrature_encoder_fetch_count(pio, sm);
}


int32_t re_count() {
    return _enc_value;
}

int16_t re_delta() {
    return _enc_delta;
}

int32_t re_tdelta() {
    return _enc_t_delta;
}

int32_t re_tlast() {
    return _enc_t_last;
}

int32_t re_velocity() {
    return _enc_velocity;
}

void re_turn_handler(uint phase) {
    if (phase % 2 == 0) {
        // Request count on EVEN
        _quadrature_encoder_request_count(PIO_ROTARY_BLOCK, PIO_ROTARY_SM);
        return;
    }
    // Get the count on ODD
    int32_t new_value;
    // note: thanks to two's complement arithmetic delta will always
    // be correct even when new_value wraps around MAXINT / MININT
    new_value = _quadrature_encoder_fetch_count(PIO_ROTARY_BLOCK, PIO_ROTARY_SM);
    // We use '-1' as a special flag to indicate that there wasn't any data available.
    if (new_value != -1) {
        _enc_delta = new_value - _enc_value;
        _enc_value = new_value;
        uint32_t now = now_ms();
        _enc_t_delta = now - _enc_t_last;
        _enc_t_last = now;
        if (_enc_delta != 0) {
            if (_enc_t_delta != 0) {
                _enc_velocity = (_enc_delta * 1000) / _enc_t_delta;
            }
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_ROTARY_CHG);
            msg.data.value16 = _enc_delta;
            postAPPMsgDiscardable(&msg);
        }
    }
}



void re_minit() {
    if (_initialized) {
        board_panic("!!! re_module_init: Called more than once !!!");
    }
    // GPIO is initialized in `board.c` with the rest of the board.
    _enc_delta = 0;
    _enc_value = 0;
    _enc_t_delta = now_ms();
    _enc_t_last = _enc_t_delta;
    uint offset = pio_add_program(PIO_ROTARY_BLOCK, &quadrature_encoder_program);
    _quadrature_encoder_program_init(PIO_ROTARY_BLOCK, PIO_ROTARY_SM, offset, _PIN_rotary_ENC_AB, 20000);

    _initialized = true;
}

