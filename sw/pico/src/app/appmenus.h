/**
 * APPMENU Menus Module.
 *
 * Application menus (for the display)
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef APPMENU_H_
#define APPMENU_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "menumgr_t.h"

extern const smenu_t app_main_menu;

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void appmenu_minit();

#ifdef __cplusplus
}
#endif
#endif // APPMENU_H_
