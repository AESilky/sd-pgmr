/**
 * @brief Application Operations (user facing)
 * @file appops.h
 * @ingroup app
 *
 * Copyright 2025 AESilky
 * SPDX-License-Identifier: MIT License
 */
#ifndef APPOPS_H_
#define APPOPS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "appmenus.h"

// Operation Methods

extern bool appop_handle_empty(const smenu_t* menu, const smenu_item_t* item);

extern bool appop_handle_eraseall(const smenu_t* menu, const smenu_item_t* item);

extern bool appop_handle_erasesect(const smenu_t* menu, const smenu_item_t* item);

extern bool appop_handle_info(const smenu_t* menu, const smenu_item_t* item);

extern bool appop_handle_verify(const smenu_t* menu, const smenu_item_t* item);

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void appops_modinit();

#ifdef __cplusplus
}
#endif
#endif // APPOPS_H_
