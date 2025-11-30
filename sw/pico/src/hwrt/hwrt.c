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
#include "picoutil.h"

#include "cmt.h"
#include "app.h"
#include "include/util.h"

#include "dskops/dskops.h"
#include "rotary_encoder/include/re_pbsw.h"
#include "rotary_encoder/include/rotary_encoder.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/float.h"
#include "pico/printf.h"

#define _HWRT_STATUS_PULSE_PERIOD 6999

static volatile bool _apps_started = false;

typedef bool (*sw_pressed_fn)(void);

static void _handle_switch0_longpress_delay(cmt_msg_t* msg);
static void _handle_switch1_longpress_delay(cmt_msg_t* msg);

static volatile bool _sw_delay_ip[_SW_CNT] = {false, false};
static msg_handler_fn _sw_longpress_delay[_SW_CNT] = { _handle_switch0_longpress_delay, _handle_switch1_longpress_delay };
static volatile bool _sw_pressed[_SW_CNT];
static sw_pressed_fn _sw_pressed_fn[_SW_CNT] = { cmdattn_switch_pressed, rotary_switch_pressed };

// Interrupt handler methods...
static void _gpio_irq_handler(uint gpio, uint32_t events);
static void _sw_irq_handler(switch_id_t sw, uint32_t events);

// Message handler methods...
static void _handle_hwrt_housekeeping(cmt_msg_t* msg);
static void _handle_hwrt_test(cmt_msg_t* msg);
static void _handle_apps_started(cmt_msg_t* msg);


// ====================================================================
// Run after delay methods
// ====================================================================

static void _debug_usb_announce(void* data) {
    // Debugging has switched over to the USB. Say hello...
    debug_printf("DEBUG output now on the USB\n");
}

static void _debug_switch_to_usb(void* data) {
    // Switch debugging over to the USB so we can safely use the PD bus
    debug_printf("Switching DEBUG output to the USB\n");
    debug_init(DIM_STDIO_TO_USB_DIUART);
    cmt_run_after_ms(3000, _debug_usb_announce, NULL);
}


// ====================================================================
// Message handler methods
// ====================================================================

static void _handle_apps_started(cmt_msg_t* msg) {
    // The Apps (on core1) has reported that it is initialized.
    // Since we are responding to a message, it means we
    // are also initialized, so -
    //
    // Start things running.
    _apps_started = true;

    // Initialize other modules that the RT oversees.
    //
    re_pbsw_minit();  // Rotary Encoder Push-Button Switch module
    re_minit();       // Rotary Encoder (knob) module
    gpio_set_irq_enabled_with_callback(IRQ_ROTARY_SW, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, _gpio_irq_handler);
    gpio_set_irq_enabled(IRQ_CMD_ATTN_SW, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Let the USB subsystem have some time to come up, then
    // Switch debugging over to the USB so we can safely use the PD bus
    cmt_run_after_ms(800, _debug_switch_to_usb, NULL);
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

    // Request the rotary switch count on even times, get it on odd.
    re_turn_handler(cnt);

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

static void _sw_debounce(cmt_msg_t* msg) {
    switch_id_t sw = msg->data.sw_action.switch_id;
    _sw_pressed[sw] = _sw_pressed_fn[sw](); // See if it's still pressed
    if (_sw_pressed[sw]) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_SW_ACTION);
        msg.data.sw_action.switch_id = sw;
        msg.data.sw_action.pressed = true;
        msg.data.sw_action.longpress = false;
        msg.data.sw_action.repeat = false;
        postHWRTMsg(&msg);
        postAPPMsg(&msg);
    }
}

static void _schedule_longpress_delay(switch_id_t sw, bool repeat) {
    if (!_sw_delay_ip[sw]) {
        _sw_delay_ip[sw] = true;
        cmt_msg_t msg;
        cmt_msg_init2(&msg, MSG_SW_LONGPRESS_DELAY, _sw_longpress_delay[sw]);
        msg.data.sw_action.switch_id = sw;
        msg.data.sw_action.pressed = true;
        msg.data.sw_action.longpress = repeat;
        msg.data.sw_action.repeat = repeat;
        uint16_t delay = (repeat ? SWITCH_REPEAT_MS : SWITCH_LONGPRESS_MS);
        schedule_msg_in_ms(delay, &msg);
    }
}

static void _handle_switch_action(cmt_msg_t* msg) {
    // Handle switch actions so we can detect a long press
    // and post a message for it.
    //
    switch_id_t sw = msg->data.sw_action.switch_id;
    bool pressed = msg->data.sw_action.pressed;
    if (!pressed) {
        // Clear any long press in progress
        scheduled_msg_cancel2(MSG_SW_LONGPRESS_DELAY, _sw_longpress_delay[sw]);
        _sw_pressed[sw] = false;
        _sw_delay_ip[sw] = false;
    }
    else {
        _sw_pressed[sw] = true;
        // Start a delay timer
        _schedule_longpress_delay(sw, msg->data.sw_action.repeat);
    }
}

static void _handle_switch_longpress_delay(cmt_msg_t* msg) {
    // Handle the long press delay message to see if the switch is still pressed.
    switch_id_t sw = msg->data.sw_action.switch_id;
    _sw_delay_ip[sw] = false; // No longer in-process
    bool repeat = msg->data.sw_action.repeat;
    bool still_pressed = _sw_pressed_fn[sw]();
    if (_sw_pressed[sw] && still_pressed) {
        // Yes, the switch is still pressed after the delay
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_SW_ACTION);
        msg.data.sw_action.switch_id = sw;
        msg.data.sw_action.pressed = true;
        msg.data.sw_action.longpress = true;
        msg.data.sw_action.repeat = repeat;
        postHWRTMsg(&msg);
        postAPPMsg(&msg);
        // Schedule another delay
        _schedule_longpress_delay(sw, true);
    }
}
static void _handle_switch0_longpress_delay(cmt_msg_t* msg) {
    _handle_switch_longpress_delay(msg);
}
static void _handle_switch1_longpress_delay(cmt_msg_t* msg) {
    _handle_switch_longpress_delay(msg);
}

// ====================================================================
// Hardware operational methods
// ====================================================================


void _gpio_irq_handler(uint gpio, uint32_t events) {
    switch (gpio) {
    case IRQ_CMD_ATTN_SW:
        _sw_irq_handler(SW_ATTNCMD, events);
        break;
    case IRQ_ROTARY_SW:
        _sw_irq_handler(SW_ROTARY, events);
        break;
    }
}

static void _sw_irq_handler(switch_id_t sw, uint32_t events) {
    // The GPIO needs to be low for at least 80ms to be considered a button press.
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Delay to see if it is user input.
        // Check to see if we have already scheduled a debounce message.
        if (!scheduled_msg_exists2(MSG_SW_DEBOUNCE, _sw_debounce)) {
            cmt_msg_t msg;
            cmt_msg_init2(&msg, MSG_SW_DEBOUNCE, _sw_debounce);
            msg.data.sw_action.switch_id = sw;
            msg.data.sw_action.pressed = true;
            msg.data.sw_action.longpress = false;
            msg.data.sw_action.repeat = false;
            schedule_msg_in_ms(80, &msg);
        }
    }
    if (events & GPIO_IRQ_EDGE_RISE) {
        scheduled_msg_cancel2(MSG_SW_DEBOUNCE, _sw_debounce);
        if (_sw_pressed[sw]) {
            _sw_pressed[sw] = false;
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_SW_ACTION);
            msg.data.sw_action.switch_id = sw;
            msg.data.sw_action.pressed = false;
            msg.data.sw_action.longpress = false;
            msg.data.sw_action.repeat = false;
            postHWRTMsg(&msg);
            postAPPMsg(&msg);
        }
    }
}


// ====================================================================
// Initialization and Startup methods
// ====================================================================

static void _hwrt_minit() {
    cmt_msg_hdlr_add(MSG_APPS_STARTED, _handle_apps_started);
    cmt_msg_hdlr_add(MSG_PERIODIC_RT, _handle_hwrt_housekeeping);
    cmt_msg_hdlr_add(MSG_HWRT_TEST, _handle_hwrt_test);
    cmt_msg_hdlr_add(MSG_SW_ACTION, _handle_switch_action);
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

    // Launch the Application functionality
    //  The APP starts other 'core-1' functionality.
    start_app();
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
    _hwrt_minit();

    dskops_minit();   // Make the Disk Operations available

    //
    // Done with the Hardware Runtime Startup - Let the DSC know.
    cmt_msg_t msg2;
    cmt_msg_init(&msg2, MSG_HWRT_STARTED);
    postAPPMsg(&msg2);

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
