/**
 * Menu Manager.
 *
 * Manages a User-Interface menuing system
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef MENUMGR_H_
#define MENUMGR_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "menumgr_t.h"


extern void dynmenu_enter(const dynmenu_t* menu);

/**
 * @brief Deactivate the menu system. This allows other components to display
 *      and handle input.
 * @ingroup menumgr
 *
 */
extern void menu_deactivate();

/**
 * @brief Display (reactivate) the current menu. Used after a handler, or other
 *      component has had control.
 * @ingroup menumgr
 *
 */
extern void menu_display_current();

/**
 * @brief Display (reactivate) the 'Main' menu. Shows the Main (first) menu if one
 *      has been established.
 * @ingroup menumgr
 *
 */
extern void menu_display_main();

extern void smenu_enter(const smenu_t* menu);

extern void dlg_dismiss(dlg_ctx_t* cntx);

extern dlg_ctx_t* dlg_wait_or_cancel(int32_t ms, msg_handler_fn on_enter, msg_handler_fn on_cancel);

/**
 * @brief Display a file picker.
 * @ingroup menumgr
 *
 *
 * @param on_enter Msg Function to be called when a file is selected.
 * @param on_cancel Msg Function to be called if the dialog is cancelled.
 * @param fselbuf Buffer to store the selected file name. Must be 256 bytes.
 * @return dlg_ctx_t* Dialog pointer for the (now) active dialog.
 */
extern dlg_ctx_t* dlg_file_pick(msg_handler_fn on_enter, msg_handler_fn on_cancel, char* fselbuf);

extern dlg_ctx_t* dlg_num_input(uint8_t row, uint8_t col, uint32_t* val, uint32_t min, uint32_t max, msg_handler_fn on_enter, msg_handler_fn on_cancel);

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void menumgr_modinit();

#ifdef __cplusplus
}
#endif
#endif // MENUMGR_H_
