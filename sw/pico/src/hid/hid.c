/**
 * @brief Human Interface Device functionality.
 * @ingroup hid
 *
 * Displays status and provide the human interface functions.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "hid.h"

#include "board.h"
#include "picoutil.h"
#include "cmt/cmt.h"
#include "display/display.h"
#include "rotary_encoder/re_pbsw.h"
#include "rotary_encoder/rotary_encoder.h"

#include <stdio.h>

// ############################################################################
// Constants Definitions
// ############################################################################
//
#define HID_DISPLAY_BG              C16_BLACK


// ############################################################################
// Function Declarations
// ############################################################################
//
static void _show_psa(proc_status_accum_t* psa, int corenum);

// Interrupt handler functions...
static void _gpio_irq_handler(uint gpio, uint32_t events);
static void _cmdattr_sw_irq_handler(uint32_t events);

// Message handler functions...
static void _handle_hid_housekeeping(cmt_msg_t* msg);
//
static void _handle_input_sw_debounce(cmt_msg_t* msg);
static void _handle_rotary_change(cmt_msg_t* msg);
static void _handle_switch_action(cmt_msg_t* msg);
static void _handle_switch_longpress_delay(cmt_msg_t* msg);


// ############################################################################
// Data
// ############################################################################
//
static switch_id_t _sw_pressed = SW_NONE;
static cmt_msg_t _sw_longpress_msg = { MSG_SW_LONGPRESS_DELAY };
static bool _cmdattn_sw_pressed;
static cmt_msg_t _stop_sw_debounce_msg = { MSG_SW_DEBOUNCE };


// ====================================================================
// Interrupt (irq) handler functions
// ====================================================================

static void _gpio_irq_handler(uint gpio, uint32_t events) {
    switch (gpio) {
    case IRQ_CMD_ATTN_SW:
        _cmdattr_sw_irq_handler(events);
        break;
    case IRQ_ROTARY_TURN:
        re_turn_irq_handler(gpio, events);
        break;
    }
}

static void _cmdattr_sw_irq_handler(uint32_t events) {
    // The GPIO needs to be low for at least 80ms to be considered a button press.
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Delay to see if it is user input or an IR received.
        // Check to see if we have already scheduled a debounce message.
        if (!scheduled_msg_exists(MSG_SW_DEBOUNCE)) {
            schedule_msg_in_ms(80, &_stop_sw_debounce_msg);
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


// ############################################################################
// Message Handlers
// ############################################################################
//
static void _display_proc_status(void* data) {
    // Output the current state
    for (int i = 0; i < 2; i++) {
        proc_status_accum_t psa;
        cmt_proc_status_sec(&psa, i);
        // Display the proc status...
        _show_psa(&psa, i);
    }
    // Do 'other' status
    // Output status every 7 seconds
    cmt_run_after_ms(7000, _display_proc_status, NULL);
}

static void _handle_hid_housekeeping(cmt_msg_t* msg) {
}

static void _handle_input_sw_debounce(cmt_msg_t* msg) {
    _cmdattn_sw_pressed = cmdattn_switch_pressed(); // See if it's still pressed
    if (_cmdattn_sw_pressed) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_CMDATTN_SW_PRESS);
        postHIDMsg(&msg);
    }
}

static void _handle_rotary_change(cmt_msg_t* msg) {
    // The rotary encoder has been turned.
    int32_t rotary_cnt = re_count();
    debug_printf("RE: p:%5d d:%3hd\n", rotary_cnt, msg->data.value16);
}

static void _handle_switch_action(cmt_msg_t* msg) {
    // Handle switch actions so we can detect a long press
    // and post a message for it.
    //
    // We keep track of one switch in each bank. We assume
    // that only one switch (per bank) can be pressed at
    // a time, so we only keep track of the last one pressed.
    //
    switch_id_t sw_id = msg->data.sw_action.switch_id;
    bool pressed = msg->data.sw_action.pressed;
    if (!pressed) {
        // Clear any long press in progress
        scheduled_msg_cancel(MSG_SW_LONGPRESS_DELAY);
        _sw_pressed = SW_NONE;
    }
    else {
        // Start a delay timer
        switch_action_data_t* sad = &_sw_longpress_msg.data.sw_action;
        _sw_pressed = sw_id;
        sad->switch_id = sw_id;
        sad->pressed = true;
        sad->repeat = false;
        schedule_msg_in_ms(SWITCH_LONGPRESS_MS, &_sw_longpress_msg);
    }
}

static void _handle_switch_longpress_delay(cmt_msg_t* msg) {
    // Handle the long press delay message to see if the switch is still pressed.
    switch_id_t sw_id = msg->data.sw_action.switch_id;
    bool repeat = msg->data.sw_action.repeat;
    cmt_msg_t* slpmsg = NULL;
    if (sw_id == _sw_pressed) {
        // Prepare to post another delay msg.
        slpmsg = &_sw_longpress_msg;
    }
    else {
        sw_id = SW_NONE;
    }
    if (sw_id != SW_NONE) {
        // Yes, the same switch is still pressed
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_SW_LONGPRESS);
        msg.data.sw_action.switch_id = sw_id;
        msg.data.sw_action.pressed = true;
        msg.data.sw_action.repeat = repeat;
        postHWRTMsgDiscardable(&msg);
        postHIDMsgDiscardable(&msg);
        // Schedule another delay
        switch_action_data_t* sad = &slpmsg->data.sw_action;
        sad->switch_id = sw_id;
        sad->pressed = true;
        sad->repeat = true;
        uint16_t delay = (repeat ? SWITCH_REPEAT_MS : SWITCH_LONGPRESS_MS);
        schedule_msg_in_ms(delay, slpmsg);
    }
}


// ############################################################################
// Internal Functions
// ############################################################################
//
static void _show_psa(proc_status_accum_t* psa, int corenum) {
    long active = psa->t_active;
    int retrieved = psa->retrieved;
    int msg_id = psa->msg_longest;
    long msg_t = psa->t_msg_longest;
    int interrupt_status = psa->interrupt_status;
    float busy = (float)active / 10000.0f; // Divide by 10,000 rather than 1,000,000 for percent
    float core_temp = 0.0f; // onboard_temp_c();
    debug_printf("PSA %d: Active: % 3.2f%%\t At:%ld\tMR:%d\t Temp: %3.1f\t Msg: %03X Msgt: %ld\t Int:%08x\n", corenum, busy, active, retrieved, core_temp, msg_id, msg_t, interrupt_status);
}

// ############################################################################
// Public Functions
// ############################################################################
//


// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//
static void _module_init(void) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("!!! HID _module_init already called. !!!");
    }
    _initialized = true;

    // Add our message handlers
    cmt_msg_hdlr_add(MSG_SW_DEBOUNCE, _handle_input_sw_debounce);
    cmt_msg_hdlr_add(MSG_ROTARY_CHG, _handle_rotary_change);
    cmt_msg_hdlr_add(MSG_SW_ACTION, _handle_switch_action);
    cmt_msg_hdlr_add(MSG_SW_LONGPRESS_DELAY, _handle_switch_longpress_delay);
    cmt_msg_hdlr_add(MSG_PERIODIC_RT, _handle_hid_housekeeping);

    re_pbsw_module_init();
    rotary_encoder_module_init();
    gpio_set_irq_enabled_with_callback(IRQ_ROTARY_TURN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &_gpio_irq_handler);
    // gpio_set_irq_enabled(IRQ_rotary_SW, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    // gpio_set_irq_enabled(IRQ_TOUCH, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Initialize the display
    display_module_init();
}

void start_hid(void) {
    // Initialize modules used by the HID
    _module_init();

    // Setup the screen.
    display_clear(Paint);
    display_string(0, 0, "Line 0.8901234567", false, Paint);
    display_string(1, 1, "SD Programmer", false, Paint);
    display_string(4, 1, "(C)2023-25", false, Paint);
    display_string(5, 2, "AESilky", false, Paint);
    //
    // Output status every 7 seconds
    cmt_run_after_ms(7000, _display_proc_status, NULL);
}
