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

static dlg_ctx_t* _dialog;
static uint32_t _sect;
static char _fileselected[256];

// ====================================================================
// Run-After/Delay/Sleep Methods
// ====================================================================

/**
 * @brief Reactivate the menu on the display. Called after delay after command ends.
 *
 * @param data Nothing important (can be pointer to anything needed)
 */
static void _reactivate_main_menu(void* data) {
    menu_display_main();
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
 * @brief Handle cancel dialog.
 *
 * @param msg .
 */
static void _on_dlg_cancel(cmt_msg_t* msg) {
    dlg_dismiss(_dialog);
    _dialog = NULL;
    cmt_run_after_ms(80, _reactivate_main_menu, NULL);
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
    dlg_dismiss(_dialog);
    _dialog = NULL;
    pdusr_erase_sect(info, _sect, false);
_finally:
    if (err) {
        // Try to turn the power off
        pdusr_pwr_request_on(false, false);
    }
}

/**
 * @brief Handle Program File Select dialog.
 *
 * @param msg .
 */
static void _on_program_fs(cmt_msg_t* msg) {
    dlg_dismiss(_dialog);
    _dialog = NULL;
    pdusr_prog(_fileselected, false);
}

/**
 * @brief Handle Verify File Select dialog.
 *
 * @param msg .
 */
static void _on_verify_fs(cmt_msg_t* msg) {
    dlg_dismiss(_dialog);
    _dialog = NULL;
    pdusr_verify(_fileselected, false);
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
    bool retval = false;
    const md_info_t* info = pdusr_info(true, false);
    if (!info) {
        retval = true;
        goto _finally;
    }
    // We need a sector number
    menu_deactivate();
    display_clear(true);
    display_line(2, "Sect:", DISP_JUSTIFY_LEFT, false, false, Paint);
    _sect = 0;
    _dialog = dlg_num_input(2, 5, &_sect, 0, (info->sectcnt - 1), _on_sect_value, _on_dlg_cancel);
_finally:
    return (retval);
}

bool appop_handle_empty(const smenu_t* menu, const smenu_item_t* item) {
    pdusr_is_empty(false);
    return (false);
}

bool appop_handle_info(const smenu_t* menu, const smenu_item_t* item) {
    pdusr_info(false, false);
    return (false);
}

bool appop_handle_program(const smenu_t* menu, const smenu_item_t* item) {
    // We need a file name to verify against.
    bool retval = false;
    const md_info_t* info = pdusr_info(true, false);
    if (!info) {
        retval = true;
        goto _finally;
    }
    // We need a file name
    menu_deactivate();
    display_clear(true);
    _dialog = dlg_file_pick(_on_program_fs, _on_dlg_cancel, _fileselected);
_finally:
    return (retval);
}

bool appop_handle_verify(const smenu_t* menu, const smenu_item_t* item) {
    // We need a file name to verify against.
    bool retval = false;
    const md_info_t* info = pdusr_info(true, false);
    if (!info) {
        retval = true;
        goto _finally;
    }
    // We need a file name
    menu_deactivate();
    display_clear(true);
    _dialog = dlg_file_pick(_on_verify_fs, _on_dlg_cancel, _fileselected);
_finally:
    return (retval);
}


// ====================================================================
// Initialization Methods
// ====================================================================

void appops_modinit() {
    if (_initialized) {
        board_panic("!!! appops_modinit: Called more than once !!!");
    }
    _initialized = true;

    pdusr_modinit();
}

