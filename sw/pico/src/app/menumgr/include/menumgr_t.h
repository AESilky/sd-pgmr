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

typedef struct mnu_dmenu_s_ dynmenu_t;
typedef struct mnu_smenu_s_ smenu_t;
typedef struct mnu_ditem_s_ dynmenu_item_t;
typedef struct mnu_sitem_s_ smenu_item_t;

typedef enum mnu_menu_type_ {
    MENU_STATIC = 0,
    MENU_DYNAMIC,
} menu_type_t;

typedef enum mnu_itemreq_type_ {
    MI_PRIOR = -1,
    MI_NEXT = 1,
} menu_itemreq_t;

/**
 * @brief Function prototype for a Dynamic Menu `get_title` function.
 * @ingroup menumgr
 *
 * @param menu Pointer to the dynmenu_t menu needing a title.
 * @return String to be used as the menu title.
 */
typedef const char* (*mnu_get_title_fn)(const dynmenu_t* menu);

/**
 * @brief Function prototype for a Dynamic Menu `get_item` function.
 * @ingroup menumgr
 *
 * @param menu Pointer to the menu the item is for.
 * @param item Pointer to a menu item, or NULL if this is for the first item.
 * @param reqtype Type of request. MI_NEXT to get the next item. MI_PRIOR to get the previous item.
 * @return dynmenu_item_t* Pointer to the item or NULL if there are no more items.
 */
typedef const dynmenu_item_t* (*mnu_get_item_fn)(const dynmenu_t* menu, const dynmenu_item_t* item, menu_itemreq_t reqtype);

/**
 * @brief Function prototype for a Dynamic Menu Item `get_item_label` function.
 * @ingroup menumgr
 *
 * @param item The dynmenu_item_t* to return the label for.
 * @return const char* Item label string.
 */
typedef const char* (*mnu_get_item_lbl_fn)(const dynmenu_t* menu, const dynmenu_item_t* item);

/**
 * @brief Function prototype for a Dynamic Menu Item `handler` function.
 * @ingroup menumgr
 *
 * @param menu Pointer to the menu this item is in.
 * @param item Pointer to the dynmenu_item_t item to handle.
 * @return bool True to reactivate the menu. False to remain inactive.
 */
typedef bool (*mnu_handle_ditem_fn)(const dynmenu_t* menu, const dynmenu_item_t* item);

/**
 * @brief Function prototype for a Static Menu Item `handler` function.
 * @ingroup menumgr
 *
 * @param menu Pointer to the menu this item is in.
 * @param item Pointer to the smenu_item_t item to handle.
 * @return bool True to reactivate the menu. False to remain inactive.
 */
typedef bool (*mnu_handle_sitem_fn)(const smenu_t* menu, const smenu_item_t* item);

/**
 * @brief Function prototype for a Dynamic Menu `has_item` function.
 * @ingroup menumgr
 *
 * @param menu Pointer to the menu the item is for.
 * @param item Pointer to a menu item, or NULL if this is for the first item.
 * @param reqtype Type of request. MI_NEXT to get the next item. MI_PRIOR to get the previous item.
 * @return bool True if the indicated item exists. False if not.
 */
typedef bool (*mnu_has_item_fn)(const dynmenu_t* menu, const dynmenu_item_t* item, menu_itemreq_t reqtype);

struct mnu_dmenu_s_ {
    menu_type_t type; // Type must be 1st and set to 'MENU_DYNAMIC'
    void* data; // Can be anything of meaning to the item functions
    const mnu_get_title_fn get_title;
    const mnu_get_item_fn get_item;
    const mnu_has_item_fn has_item;
};

struct mnu_ditem_s_ {
    void* data; // Can be anything of meaning to the item functions
    const mnu_get_item_lbl_fn get_label;
    const mnu_handle_ditem_fn handler;
};

struct mnu_smenu_s_ {
    menu_type_t type; // Type must be 1st and set to 'MENU_STATIC'
    void* data; // Can be anything of meaning to the item functions
    const char* title;
    const smenu_item_t** items;
};

struct mnu_sitem_s_ {
    void* data; // Can be anything of meaning to the item functions
    const char* label;
    const mnu_handle_sitem_fn handler;
};

/**
 * @brief Dynamic Menu Framework (types)
 * @ingroup menumgr
 *
 * The following structures/types can be used to create a dynamic menu.
 * Functions need to be defined to handle the various methods.
 *
 * An example of a set of functions to handle a dynamic menu are:
 *  static const dynmenu_item_t* _dm_get_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
 *  static const char* _dm_get_item_lbl(const dynmenu_t* menu, const dynmenu_item_t* item);
 *  static const char* _dm_get_title(const dynmenu_t* menu);
 *  static bool _dm_handle_item(const dynmenu_t* menu, const dynmenu_item_t* item);
 *  static bool _dm_has_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
 *
 */

/**
 * @brief Dynamic Menu Item Content.
 */
typedef struct dynmenu_item_content_ {
    const char* label;
    const dynmenu_item_t* item;
} dynmenu_item_content_t;

/**
 * @brief Dynamic Menu Content.
 */
typedef struct dynmenu_content_ {
    const char* title;
    const dynmenu_t* menu;
    const dynmenu_item_content_t** items;
} dynmenu_content_t;


#ifdef __cplusplus
}
#endif
#endif // MENUMGR_T_H_
