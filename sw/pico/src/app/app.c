/**
 * @brief Application functionality.
 * @ingroup app
 *
 * Higher level application functions.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */

#include "app.h"

#include "board.h"
#include "debug_support.h"
#include "util.h"

#include "shell/cmd/cmd.h"
#include "shell/shell.h"
#include "deviceops/prog_device.h"
#include "deviceops/pdops.h"
#include "dskops/dskops.h"
#include "hwrt/hwrt_t.h"
#include "picohlp/picoutil.h"
#include "cmt/cmt.h"
#include "display/display.h"
#include "rotary_encoder/rotary_encoder.h"

#include <stdio.h>

// ############################################################################
// Constants Definitions
// ############################################################################
//
#define APP_DISPLAY_BG              C16_BLACK


// ############################################################################
// Function Declarations
// ############################################################################
//
static void _show_psa(proc_status_accum_t* psa, int corenum);

// Message handler functions...
static void _handle_app_housekeeping(cmt_msg_t* msg);
//
static void _handle_rotary_change(cmt_msg_t* msg);
static void _handle_switch_action(cmt_msg_t* msg);


// ############################################################################
// Data
// ############################################################################
//


// ====================================================================
// Interrupt (irq) handler functions
// ====================================================================


// ############################################################################
// 'Run After' Methods
// ############################################################################
//

/**
 * @brief Called after delay after start up to clear off the welcome screen.
 *
 * After this, enable user input.
 *
 * @param data Nothing important
 */
static void _clear_and_enable_input(void* data) {
    display_clear(Paint);
    //
    // Enable the user input controls...
    // ZZZ
    int disprow = 0;
    display_string(disprow++, 0, "  Main Menu   ", false, true, Paint);

    // Try to mount the SD Card and display the top level files/directories
    FRESULT fr = dsk_mount_sd();
    if (fr != FR_OK) {
        error_printf("Cannot mount SD  FR: %d - %s\n", fr, FRESULT_str(fr));
        return;
    }
    DIR dir;
    FILINFO finfo;
    // Open the root dir
    char* dirpath = "/";
    fr = f_opendir(&dir, dirpath);
    if (fr != FR_OK) {
        error_printf("Cannon open dir: '%s'  FR: %d - %s\n", dirpath, fr, FRESULT_str(fr));
        return;
    }
    // Get the first file
    fr = f_findfirst(&dir, &finfo, dirpath, "*");
    if (fr != FR_OK) {
        error_printf("Cannon read dir (ff): '%s'  FR: %d - %s\n", dirpath, fr, FRESULT_str(fr));
        return;
    }
    if (finfo.fattrib & AM_DIR) {
        strcat((char*)&finfo.fname, "/");
    }
    display_string(disprow++, 0, finfo.fname, false, false, Paint);
    // Get the rest...
    do {
        fr = f_findnext(&dir, &finfo);
        if (fr != FR_OK) {
            error_printf("Cannon read dir (nf): '%s'  FR: %d - %s\n", dirpath, fr, FRESULT_str(fr));
            return;
        }
        if (finfo.fattrib & AM_DIR) {
            strcat((char*)&finfo.fname, "/");
        }
        display_string(disprow++, 0, finfo.fname, false, false, Paint);
    }
    while (disprow < display_info().rows);


    // Initialize the shell
    shell_module_init();
    //
    // Built the shell
    shell_build();
    term_text_normal();
    // Activate the command processor
    cmd_activate(true);
}

static void _display_proc_status(void* data) {
    // Output the current state
    for (int i = 0; i < 2; i++) {
        proc_status_accum_t psa;
        cmt_proc_status_sec(&psa, i);
        // Display the proc status...
        _show_psa(&psa, i);
    }
    // Do 'other' status
    // Output status every 16 seconds
    cmt_run_after_ms(Seconds_ms(16), _display_proc_status, NULL);
}


// ############################################################################
// Message Handlers
// ############################################################################
//
static void _handle_app_housekeeping(cmt_msg_t* msg) {
}

static void _handle_rotary_change(cmt_msg_t* msg) {
    // The rotary encoder has been turned.
    int32_t rotary_cnt = re_count();
    int16_t rotary_delta = re_delta();
    int32_t t_last = re_tlast();
    int32_t t_delta = re_tdelta();
    debug_printf("RE: p:%4d d:%3hd dt:%3u t:%6u  md:%3hd\n", rotary_cnt, rotary_delta, t_delta, t_last, msg->data.value16);
}

static void _handle_switch_action(cmt_msg_t* msg) {
    // Handle switch actions for the menuing
    //
    switch_id_t sw = msg->data.sw_action.switch_id;
    bool pressed = msg->data.sw_action.pressed;
    bool longpress = msg->data.sw_action.longpress;
    bool repeat = msg->data.sw_action.repeat;

    // ZZZ - TEMP: Use the CMD-ATTN switch to toggle the Flash-Device power...
    if (sw == SW_ATTNCMD && pressed && !longpress && !repeat) {
        pdo_pwr_on(!pdo_pwr_is_on());
    }

    // DEBUG: Print info about the switch action.
    char* spressed = (pressed ? "Pressed" : "Released");
    if (longpress) {
        spressed = "Long-Pressed";
    }
    char* srepeat = (repeat ? "Repeat..." : "");
    char* sw_name = (sw == SW_ATTNCMD ? "CmdAttn" : "Rotary");
    debug_printf("%s %s %s\n", sw_name, spressed, srepeat);
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
        board_panic("!!! APP _module_init already called. !!!");
    }
    _initialized = true;

    // Add our message handlers
    cmt_msg_hdlr_add(MSG_ROTARY_CHG, _handle_rotary_change);
    cmt_msg_hdlr_add(MSG_SW_ACTION, _handle_switch_action);
    cmt_msg_hdlr_add(MSG_PERIODIC_RT, _handle_app_housekeeping);

    pd_module_init();       // Programmable Device (Flash) module

    // Initialize the display
    display_module_init(true); // Initialize, and invert the display (as it is mounted upside down)
}

void start_app(void) {
    // Initialize modules used by the APP
    _module_init();

    // Setup the screen.
    display_clear(Paint);
    display_string(0, 1, "SilkyDESIGN", false, false, Paint);
    display_string(1, 2, "Programmer", false, false, Paint);
    display_string(4, 3, "\0012023-25", false, false, Paint);
    display_string(5, 3, "AESilky", false, false, Paint);
    //
    // Clear the display and enable user input after 5 seconds.
    cmt_run_after_ms(2000, _clear_and_enable_input, NULL);

    //
    // Output status every 7 seconds
    cmt_run_after_ms(7000, _display_proc_status, NULL);

    //
    // Done with Apps Startup - Let the Runtime know.
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_APPS_STARTED);
    postHWRTMsg(&msg);

}
