/**
 * APPMENU Menus Module.
 *
 * Application menus (for the display)
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "appmenus.h"
#include "appops.h"
#include "menumgr.h"
#include "app.h"

#include "board.h"

// Menu methods
static const dynmenu_item_t* _dm_get_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
static const char* _dm_get_item_lbl(const dynmenu_t* menu, const dynmenu_item_t* item);
static const char* _dm_get_title(const dynmenu_t* menu);
static bool _dm_handle_item(const dynmenu_t* menu, const dynmenu_item_t* item);
static bool _dm_has_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
//
static bool _mm_handle_item(const smenu_t* menu, const smenu_item_t* item);

// Main Menu (static menu)
static const smenu_item_t _mm_item1 = { .label = "Info", .handler = appop_handle_info, .data = (void*)0 };
static const smenu_item_t _mm_item2 = { .label = "Is Blank", .handler = appop_handle_empty, .data = (void*)1 };
static const smenu_item_t _mm_item3 = { .label = "Program", .handler = _mm_handle_item, .data = (void*)2 };
static const smenu_item_t _mm_item4 = { .label = "Verify", .handler = _mm_handle_item, .data = (void*)3 };
static const smenu_item_t _mm_item5 = { .label = "Erase Sect", .handler = appop_handle_erasesect, .data = (void*)4 };
static const smenu_item_t _mm_item6 = { .label = "Erase Device", .handler = appop_handle_eraseall, .data = (void*)5 };
static const smenu_item_t* _mm_items[] = {
    &_mm_item1,
    &_mm_item2,
    &_mm_item3,
    &_mm_item4,
    &_mm_item5,
    &_mm_item6,
    NULL
};
const smenu_t app_main_menu = { .type = MENU_STATIC, .title = "Device", .items = _mm_items, .data = NULL };

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
static const dynmenu_item_content_t* _dm_items[] = { &_dm_item1, &_dm_item2, &_dm_item3, &_dm_item4, &_dm_item5, &_dm_item6, &_dm_item7, NULL };
static const dynmenu_t  _dynamic_menu;
static dynmenu_content_t  _dynamic_menu_c = { .title = "Dynamic Menu", .items = _dm_items, .menu = &_dynamic_menu };
static const dynmenu_t  _dynamic_menu = { .type = MENU_DYNAMIC, .get_title = _dm_get_title, .get_item = _dm_get_item, .has_item = _dm_has_item, .data = (void*)&_dynamic_menu_c };


static const dynmenu_item_t* _dm_get_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype) {
    const dynmenu_content_t* mc = (dynmenu_content_t*)menu->data;
    int piid = (ref_item ? (int)(ref_item->data) : -1);
    const dynmenu_item_content_t* mic = (dynmenu_item_content_t*)mc->items[piid + 1];
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

