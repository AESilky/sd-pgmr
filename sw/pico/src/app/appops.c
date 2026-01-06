/**
 * @brief Application Operations (user facing)
 * @file appops.h
 * @ingroup app
 *
 * Copyright 2025 AESilky
 * SPDX-License-Identifier: MIT License
 */
#include "appops.h"
#include "pdusr.h"

#include "board.h"
#include "cmt.h"
#include "display.h"
#include "menumgr.h"

// ====================================================================
// Data Section
// ====================================================================

static volatile bool _initialized;

static dlg_ctx_t* _dlgctx;
static uint32_t _sect;

// ====================================================================
// Run-After/Delay/Sleep Methods
// ====================================================================

/**
 * @brief Reactivate the menu on the display. Called after delay after command ends.
 *
 * @param data Nothing important (can be pointer to anything needed)
 */
static void _reactivate_menu(void* data) {
    menu_display_current();
}


// ====================================================================
// Message Handler Methods
// ====================================================================

/**
 * @brief Handle Erase Device.
 *
 * @param msg .
 */
static void _do_erase_all(cmt_msg_t* msg) {
    pdusr_erase_all(false);
}

/**
 * @brief Handle sector value enter.
 *
 * @param msg .
 */
static void _on_sect_value(cmt_msg_t* msg) {
    const md_info_t* info = pdusr_info(true, false);
    bool err = false;
    if (!info) {
        display_line(2, "Device Error", DISP_JUSTIFY_CENTER, false, false, Paint);
        err = true;
        goto _finally;
    }
    if (info->sectcnt <= _sect) {
        display_line(2, "Invalid Sect", DISP_JUSTIFY_CENTER, false, false, Paint);
        err = true;
        goto _finally;
    }
    dlg_dismiss(_dlgctx);
    pdusr_erase_sect(info, _sect, false);
_finally:
    if (err) {
        // Try to turn the power off
        pdusr_pwr_request_on(false, false);
        // Delay and reenable the menu.
        cmt_run_after_ms(8000, _reactivate_menu, NULL);
    }
}


// ====================================================================
// Public Methods
// ====================================================================

bool appop_handle_eraseall(const smenu_t* menu, const smenu_item_t* item) {
    cmt_msg_t msg;
    cmt_exec_init(&msg, _do_erase_all);
    postAPPMsg(&msg);
    return (false);
}

bool appop_handle_erasesect(const smenu_t* menu, const smenu_item_t* item) {
    const md_info_t* info = pdusr_info(true, false);
    if (!info) {
        goto _finally;
    }
    // We need a sector number
    menu_deactivate();
    display_clear(true);
    display_line(2, "Sect:", DISP_JUSTIFY_LEFT, false, false, Paint);
    _sect = 0;
    _dlgctx = dlg_num_input(2, 5, &_sect, 0, (info->sectcnt - 1), _on_sect_value);
_finally:
    return (false);
}

bool appop_handle_empty(const smenu_t* menu, const smenu_item_t* item) {
    pdusr_is_empty(false);
    return (false);
}

bool appop_handle_info(const smenu_t* menu, const smenu_item_t* item) {
    pdusr_info(false, false);
    return (false);
}


// ====================================================================
// Initialization Methods
// ====================================================================

void appops_minit() {
    if (_initialized) {
        board_panic("!!! appops_minit: Called more than once !!!");
    }
    _initialized = true;

    pdusr_minit();
}

