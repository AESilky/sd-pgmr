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


extern void menu_enter(const menu_t* menu);

extern void menu_main_set(const menu_t* menu);

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void menumgr_minit();

#ifdef __cplusplus
}
#endif
#endif // MENUMGR_H_
