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

extern void smenu_enter(const smenu_t* menu);

extern void dlg_dismiss(dlg_ctx_t* cntx);

extern dlg_ctx_t* dlg_num_input(uint8_t row, uint8_t col, uint32_t* val, uint32_t min, uint32_t max, msg_handler_fn on_change);

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void menumgr_minit();

#ifdef __cplusplus
}
#endif
#endif // MENUMGR_H_
