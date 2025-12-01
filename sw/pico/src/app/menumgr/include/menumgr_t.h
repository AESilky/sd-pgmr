/**
 * Menu Manager - Types, Structures, ENUMs.
 *
 * Manages a User-Interface menuing system
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef MENUMGR_T_H_
#define MENUMGR_T_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "pico/types.h" // 'uint' and other standard types

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct mnu_menu_s_ menu_t;
typedef struct mnu_item_s_ menu_item_t;

/**
 * @brief Function prototype for a Menu Handler `mnu_get_title` function.
 * @ingroup menumgr
 *
 * @param menu Pointer to the menu_t menu needing a title.
 * @return String to be used as the menu title.
 */
typedef const char* (*mnu_get_title_fn)(const menu_t* menu);

/**
 * @brief Function prototype for a Menu Handler `get_item` function.
 * @ingroup menumgr
 *
 * @param menu Pointer to the menu the item is for.
 * @param prev_item Pointer to the previous menu item, or NULL if this is the first item.
 * @return menu_item_t* Pointer to the item or NULL if there are no more items.
 */
typedef const menu_item_t* (*mnu_get_item_fn)(const menu_t* menu, const menu_item_t* prev_item);

/**
 * @brief Function prototype for a Menu Item Handler `get_item_label` function.
 * @ingroup menumgr
 *
 * @param item The menu_item_t* to return the label for.
 * @return const char* Item label string.
 */
typedef const char* (*mnu_get_item_lbl_fn)(const menu_t* menu, const menu_item_t* item);

/**
 * @brief Function prototype for a Menu Handler `mnu_handle_item` function.
 * @ingroup menumgr
 *
 * @param item Pointer to the menu_item_t item to handle.
 */
typedef void (*mnu_handle_item_fn)(const menu_t* menu, const menu_item_t* item);

/**
 * @brief Function prototype for a Menu Handler `has_item` function.
 * @ingroup menumgr
 *
 * @param menu Pointer to the menu the item is for.
 * @param prev_item Pointer to the previous menu item, or NULL if this is the first item.
 * @return bool True if the indicated item exists. False if not.
 */
typedef bool (*mnu_has_item_fn)(const menu_t* menu, const menu_item_t* prev_item);

struct mnu_menu_s_ {
    const mnu_get_title_fn get_title;
    const mnu_get_item_fn get_item;
    const mnu_has_item_fn has_item;
    void* data; // Can be anything of meaning to the item functions
};

struct mnu_item_s_ {
    const mnu_get_item_lbl_fn get_label;
    const mnu_handle_item_fn handler;
    void* data; // Can be anything of meaning to the item functions
};

#ifdef __cplusplus
}
#endif
#endif // MENUMGR_T_H_
