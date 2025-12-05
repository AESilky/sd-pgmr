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
#include "include/util.h"

#include "shell.h"
#include "hwrt_t.h"
#include "menumgr.h"
#include "picoutil.h"
#include "cmt.h"
#include "display.h"
#include "dskops/dskops.h"
#include "rotary_encoder/include/rotary_encoder.h"

#include "deviceops/include/prog_device.h"
#include "deviceops/include/pdops.h"

#include <stdio.h>

// ############################################################################
// Constants Definitions
// ############################################################################
//
#define APP_DISPLAY_BG              C16_BLACK


// ############################################################################
// Datatypes
// ############################################################################
//


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

// Menu methods
static const dynmenu_item_t* _dm_get_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
static const char* _dm_get_item_lbl(const dynmenu_t* menu, const dynmenu_item_t* item);
static const char* _dm_get_title(const dynmenu_t* menu);
static bool _dm_handle_item(const dynmenu_t* menu, const dynmenu_item_t* item);
static bool _dm_has_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
//
static bool _mm_handle_item(const smenu_t* menu, const smenu_item_t* item);


// ############################################################################
// Data
// ############################################################################
//
int ERRORNO;    // Primarily used by the Shell and Shell Commands. Globally available error number.

// Main Menu (static menu)
static const smenu_item_t _mm_item1 = {.label = "Device", .handler = _mm_handle_item, .data = (void*)0 };
static const smenu_item_t _mm_item2 = { .label = "File", .handler = _mm_handle_item, .data = (void*)1 };
static const smenu_item_t _mm_item3 = { .label = "Host", .handler = _mm_handle_item, .data = (void*)2 };
static const smenu_item_t _mm_item4 = { .label = "About", .handler = _mm_handle_item, .data = (void*)3 };
static const smenu_item_t* _mm_items[] = {&_mm_item1, &_mm_item2, &_mm_item3, &_mm_item4, NULL };
static const smenu_t _main_menu = {.type = MENU_STATIC, .title = "Main Menu", .items = _mm_items, .data = NULL };

// Dynamic menu
static const dynmenu_item_t _ditem1 = { .get_label = _dm_get_item_lbl, .handler = _dm_handle_item, .data = (void*)0 };
static const dynmenu_item_t _ditem2 = { .get_label = _dm_get_item_lbl, .handler = _dm_handle_item, .data = (void*)1 };
static const dynmenu_item_t _ditem3 = { .get_label = _dm_get_item_lbl, .handler = _dm_handle_item, .data = (void*)2 };
static const dynmenu_item_t _ditem4 = { .get_label = _dm_get_item_lbl, .handler = _dm_handle_item, .data = (void*)3 };
static const dynmenu_item_t _ditem5 = { .get_label = _dm_get_item_lbl, .handler = _dm_handle_item, .data = (void*)4 };
static const dynmenu_item_t _ditem6 = { .get_label = _dm_get_item_lbl, .handler = _dm_handle_item, .data = (void*)5 };
static const dynmenu_item_t _ditem7 = { .get_label = _dm_get_item_lbl, .handler = _dm_handle_item, .data = (void*)6 };
static const dynmenu_item_content_t _dm_item1 = { .label = "Item 1", .item = &_ditem1 };
static const dynmenu_item_content_t _dm_item2 = { .label = "Item 2", .item = &_ditem2 };
static const dynmenu_item_content_t _dm_item3 = { .label = "Item 3", .item = &_ditem3 };
static const dynmenu_item_content_t _dm_item4 = { .label = "Item 4", .item = &_ditem4 };
static const dynmenu_item_content_t _dm_item5 = { .label = "Item 5", .item = &_ditem5 };
static const dynmenu_item_content_t _dm_item6 = { .label = "Item 6", .item = &_ditem6 };
static const dynmenu_item_content_t _dm_item7 = { .label = "Item 7", .item = &_ditem7 };
static const dynmenu_item_content_t* _dm_items[] = {&_dm_item1, &_dm_item2, &_dm_item3, &_dm_item4, &_dm_item5, &_dm_item6, &_dm_item7, NULL};
static const dynmenu_t  _dynamic_menu;
static dynmenu_content_t  _dynamic_menu_c = { .title = "Dynamic Menu", .items = _dm_items, .menu = & _dynamic_menu};
static const dynmenu_t  _dynamic_menu = {.type = MENU_DYNAMIC, .get_title = _dm_get_title, .get_item = _dm_get_item, .has_item = _dm_has_item, .data = (void*)& _dynamic_menu_c};

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
    // Initialize the shell
    shell_minit();
    //
    // Start the shell
    shell_start();

    //
    // Enable the user input controls...
    //
    // Display the Main Menu
    smenu_enter(& _main_menu);

    // Try to mount the SD Card and display the top level files/directories
    // FRESULT fr = dsk_mount_sd();
    // if (fr != FR_OK) {
    //     error_printf("Cannot mount SD  FR: %d - %s\n", fr, FRESULT_str(fr));
    //     return;
    // }
    // DIR dir;
    // FILINFO finfo;
    // // Open the root dir
    // char* dirpath = "/";
    // fr = f_opendir(&dir, dirpath);
    // if (fr != FR_OK) {
    //     error_printf("Cannon open dir: '%s'  FR: %d - %s\n", dirpath, fr, FRESULT_str(fr));
    //     return;
    // }
    // // Get the first file
    // fr = f_findfirst(&dir, &finfo, dirpath, "*");
    // if (fr != FR_OK) {
    //     error_printf("Cannon read dir (ff): '%s'  FR: %d - %s\n", dirpath, fr, FRESULT_str(fr));
    //     return;
    // }
    // if (finfo.fattrib & AM_DIR) {
    //     strcat((char*)&finfo.fname, "/");
    // }
    // display_string(disprow++, 0, finfo.fname, false, false, Paint);
    // // Get the rest...
    // do {
    //     fr = f_findnext(&dir, &finfo);
    //     if (fr != FR_OK) {
    //         error_printf("Cannot read dir (nf): '%s'  FR: %d - %s\n", dirpath, fr, FRESULT_str(fr));
    //         return;
    //     }
    //     if (finfo.fattrib & AM_DIR) {
    //         strcat((char*)&finfo.fname, "/");
    //     }
    //     display_string(disprow++, 0, finfo.fname, false, false, Paint);
    // }
    // while (disprow < display_info().rows);
}

static void _display_proc_status(void* data) {
    // Output the current state
    if (debug_mode_enabled()) {
        cmt_sm_counts_t smwc = scheduled_msgs_waiting();
        for (int i = 0; i < 2; i++) {
            proc_status_accum_t psa;
            cmt_proc_status_sec(&psa, i);
            // Display the proc status...
            _show_psa(&psa, i);
        }
        debug_printf("Scheduled messages: %d\n", smwc.total);
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
    if (debug_mode_enabled()) {
        int32_t rotary_cnt = re_count();
        int16_t rotary_delta = re_delta();
        int32_t t_last = re_tlast();
        int32_t t_delta = re_tdelta();
        int32_t velocity = re_velocity();
        debug_printf("RE: cnt:%4d delta:%3hd velo: %4d dt:%5u t:%8u  md:%3hd\n", rotary_cnt, rotary_delta, velocity, t_delta, t_last, msg->data.value16);
    }
}

static void _handle_switch_action(cmt_msg_t* msg) {
    // Handle switch actions for the menuing
    //
    if (debug_mode_enabled()) {
        switch_id_t sw = msg->data.sw_action.switch_id;
        bool pressed = msg->data.sw_action.pressed;
        bool longpress = msg->data.sw_action.longpress;
        bool repeat = msg->data.sw_action.repeat;

        // DEBUG: Print info about the switch action.
        char* spressed = (pressed ? "Pressed" : "Released");
        if (longpress) {
            spressed = "Long-Pressed";
        }
        char* srepeat = (repeat ? "Repeat..." : "");
        char* sw_name = (sw == SW_ATTNCMD ? "CmdAttn" : "Rotary");
        debug_printf("%s %s %s\n", sw_name, spressed, srepeat);
    }
}


// ############################################################################
// Internal Functions
// ############################################################################
//
static const dynmenu_item_t* _dm_get_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype) {
    const dynmenu_content_t* mc = (dynmenu_content_t*)menu->data;
    int piid = (ref_item ? (int)(ref_item->data) : -1);
    const dynmenu_item_content_t* mic = (dynmenu_item_content_t*)mc->items[piid+1];
    if (!mic) {
        // Previous is the last item
        return (NULL);
    }
    return (mic->item);
}

static const char* _dm_get_item_lbl(const dynmenu_t* menu, const dynmenu_item_t* item) {
    dynmenu_content_t* mc = (dynmenu_content_t*)menu->data;
    dynmenu_item_content_t* mic = (dynmenu_item_content_t*)mc->items[(int)(item->data)];
    return (mic->label);
}

static const char* _dm_get_title(const dynmenu_t* menu) {
    dynmenu_content_t* mc = (dynmenu_content_t*)menu->data;
    return (mc->title);
}

static bool _dm_handle_item(const dynmenu_t* menu, const dynmenu_item_t* item) {
    const char* title = menu->get_title(menu);
    const char* label = item->get_label(menu, item);
    int item_num = (int)item->data;
    info_printf("%s item '%s' (%d) selected.\n", title, label, item_num);
    return (true);
}

static bool _dm_has_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype) {
    return (_dm_get_item(menu, ref_item, reqtype) != NULL);
}

static bool _mm_handle_item(const smenu_t* menu, const smenu_item_t* item) {
    const char* title = menu->title;
    const char* label = item->label;
    int item_num = (int)item->data;
    info_printf("%s item '%s' (%d) selected.\n", title, label, item_num);
    return (true);
}

static void _show_psa(proc_status_accum_t* psa, int corenum) {
    long active = psa->t_active;
    float busy = (active < 1000000l ? (float)active / 10000.0f : 100.0f); // Divide by 10,000 rather than 1,000,000 for percent
    char* ts = "us";
    if (active >= 10000l) {
        active /= 1000; // Adjust to milliseconds
        ts = "ms";
    }
    int retrieved = psa->retrieved;
    int msg_id = psa->msg_longest;
    long msg_t = psa->t_msg_longest;
    int interrupt_status = psa->interrupt_status;
    debug_printf("Core %d: Active:% 3.2f%% (%ld%s)\t Msgs:%d\t LongMsgID:%02X (%ldus)\t IntFlags:%08x\n",
        corenum, busy, active, ts, retrieved, msg_id, msg_t, interrupt_status);
}


// ############################################################################
// Public Functions
// ############################################################################
//


// ############################################################################
// Initialization and Maintainence Functions
// ############################################################################
//
static void _minit(void) {
    static bool _initialized = false;

    if (_initialized) {
        board_panic("!!! APP _module_init already called. !!!");
    }
    _initialized = true;

    // Programmable Device (Flash) module
    pd_minit();

    // Add our message handlers
    cmt_msg_hdlr_add(MSG_ROTARY_CHG, _handle_rotary_change);
    cmt_msg_hdlr_add(MSG_SW_ACTION, _handle_switch_action);
    cmt_msg_hdlr_add(MSG_PERIODIC_RT, _handle_app_housekeeping);


    // Initialize the display
    display_minit(true); // Initialize, and invert the display (as it is mounted upside down)

    // Initialize the Menus and Menu Manager
    menumgr_minit();
}

void start_app(void) {
    // Initialize modules used by the APP
    _minit();

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
