/**
 * Hardware Runtime for Board-0.
 *
 * Setup for the message loop and idle processing.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "hwrt.h"

#include "board.h"
#include "debug_support.h"
#include "picohlp/picoutil.h"

#include "cmt/cmt.h"
#include "dskops/dskops.h"
#include "hid/hid.h"
#include "util/util.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/float.h"
#include "pico/printf.h"

#define _HWRT_STATUS_PULSE_PERIOD 6999

static volatile bool _dcs_started = false;

static volatile bool _cmdattn_sw_pressed;


// Interrupt handler functions...
static void _gpio_irq_handler(uint gpio, uint32_t events);
static void _cmdattn_sw_irq_handler(uint32_t events);

// Message handler functions...
static void _handle_hwrt_housekeeping(cmt_msg_t* msg);
static void _handle_hwrt_test(cmt_msg_t* msg);
static void _handle_dcs_started(cmt_msg_t* msg);


// ====================================================================
// Message handler functions
// ====================================================================

static void _handle_dcs_started(cmt_msg_t* msg) {
    // The DCS has reported that it is initialized.
    // Since we are responding to a message, it means we
    // are also initialized, so -
    //
    // Start things running.
    _dcs_started = true;
}

/**
 * @brief Handle HW Runtime Housekeeping tasks. This is triggered every ~16ms.
 *
 * For reference, 625 times is 10 seconds.
 *
 * @param msg Nothing important in the message.
 */
static void _handle_hwrt_housekeeping(cmt_msg_t* msg) {
    static uint cnt = 0;

    cnt++;
}

static void _handle_hwrt_test(cmt_msg_t* msg) {
    // Test `scheduled_msg_ms` error
    static int times = 1;

    cmt_msg_t msg_time = { MSG_HWRT_TEST };
    uint64_t period = 60;

    msg_time.data.ts_us = now_us(); // Get the 'next' -> 'last_time' fresh
    schedule_msg_in_ms((period * 1000), &msg_time);
    times++;
}

static void _handle_user_sw_debounce(cmt_msg_t* msg) {
    _cmdattn_sw_pressed = cmdattn_switch_pressed(); // See if it's still pressed
    if (_cmdattn_sw_pressed) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_CMDATTN_SW_PRESS);
        postHIDMsg(&msg);
    }
}


// ====================================================================
// Hardware operational functions
// ====================================================================


void _gpio_irq_handler(uint gpio, uint32_t events) {
    switch (gpio) {
    case IRQ_CMD_ATTN_SW:
        _cmdattn_sw_irq_handler(events);
        break;
    }
}

static void _cmdattn_sw_irq_handler(uint32_t events) {
    // The GPIO needs to be low for at least 80ms to be considered a button press.
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Delay to see if it is user input.
        // Check to see if we have already scheduled a debounce message.
        if (!scheduled_msg_exists(MSG_SW_DEBOUNCE)) {
            cmt_msg_t msg;
            cmt_msg_init2(&msg, MSG_SW_DEBOUNCE, _handle_user_sw_debounce);
            schedule_msg_in_ms(80, &msg);
        }
    }
    if (events & GPIO_IRQ_EDGE_RISE) {
        scheduled_msg_cancel(MSG_SW_DEBOUNCE);
        if (_cmdattn_sw_pressed) {
            _cmdattn_sw_pressed = false;
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_CMDATTN_SW_RELEASE);
            postHIDMsg(&msg);
        }
    }
}


// ====================================================================
// Initialization and Startup functions
// ====================================================================

static void _hwrt_module_init() {
    cmt_msg_hdlr_add(MSG_PERIODIC_RT, _handle_hwrt_housekeeping);
    cmt_msg_hdlr_add(MSG_HWRT_TEST, _handle_hwrt_test);
    cmt_msg_hdlr_add(MSG_HID_STARTED, _handle_dcs_started);


    _cmdattn_sw_pressed = false;
    gpio_set_irq_enabled_with_callback(CMD_ATTN_SW_GPIO, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &_gpio_irq_handler);
}

/**
 * @brief Will be called by the CMT from the Core-1 message loop processor
 * when the message loop is running.
 *
 * @param msg Nothing important in the message
 */
static void _core1_started(cmt_msg_t* msg) {
    static bool _core1_started = false;
    // Make sure we aren't already started and that we are being called from core-0.
    if (_core1_started || 1 != get_core_num()) {
        board_panic("!!! `_core1_started` called more than once or on the wrong core. Core is: %hhd !!!", get_core_num());
    }
    _core1_started = true;

    // Launch the Human Interface Devices system
    //  The HID starts other 'core-1' functionality.
    start_hid();
}

/**
 * @brief For Board-0, The `core1_main` kicks off the message loop with the DCS
 * as the primary functionality.
 *
 */
void core1_main() {
    static bool _core1_main_called = false;
    // Make sure we aren't already called and that we are being called from core-1.
    if (_core1_main_called || 1 != get_core_num()) {
        board_panic("!!! `core1_main` called more than once or on the wrong core. Core is: %hhd !!!", get_core_num());
    }
    _core1_main_called = true;
    info_printf("\nCORE-%d - *** Started ***\n", get_core_num());

    // Enter into the (endless) Message Dispatching Loop
    message_loop(_core1_started);
}

/**
 * @brief Will be called by the CMT from the Core-0 message loop processor
 * when the message loop is running.
 *
 * @param msg Nothing important in the message
 */
static void _hwrt_started(cmt_msg_t* msg) {
    // Initialize now that the message loop is running.
    _hwrt_module_init();

    // Initialize other modules that the RT will oversee.
    //
    dskops_module_init(); // Make the Disk Operations available

    //
    // Done with the Hardware Runtime Startup - Let the DSC know.
    cmt_msg_t msg2;
    cmt_msg_init(&msg2, MSG_HWRT_STARTED);
    postHIDMsg(&msg2);

    // Post a TEST to ourself in case we have any tests set up.
    cmt_msg_t msg3;
    cmt_msg_init(&msg3, MSG_HWRT_TEST);
    postHWRTMsgDiscardable(&msg3);
}

void start_hwrt() {
    static bool _started = false;
    // Make sure we aren't already started and that we are being called from core-0.
    if(_started || 0 != get_core_num()) {
        board_panic("!!! `start_hwrt` called more than once or on the wrong core. Core is: %hhd !!!", get_core_num());
    }
    _started = true;

    // Enter into the message loop.
    message_loop(_hwrt_started);
}
