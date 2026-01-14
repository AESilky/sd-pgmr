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
#include "dskops.h"
#include "shell.h"
#include "include/util.h"

#include "pico/malloc.h"
#include <stdlib.h>
#include <string.h>

// ====================================================================
// Data Section
// ====================================================================

static volatile bool _initialized;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================


// Dynamic Menu Sample methods
static const dynmenu_item_t* _dm_get_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
static const char* _dm_get_item_lbl(const dynmenu_t* menu, const dynmenu_item_t* item);
static const char* _dm_get_title(const dynmenu_t* menu);
static bool _dm_handle_item(const dynmenu_t* menu, const dynmenu_item_t* item);
static bool _dm_has_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
//
// Main Menu Handler (for unimplemented items)
static bool _mm_handle_item(const smenu_t* menu, const smenu_item_t* item);

// Main Programmer Menu (static menu)
static const smenu_item_t _mm_item1 = { .label = "Info", .handler = appop_handle_info, .data = (void*)0 };
static const smenu_item_t _mm_item2 = { .label = "Is Blank", .handler = appop_handle_empty, .data = (void*)1 };
static const smenu_item_t _mm_item3 = { .label = "Program", .handler = appop_handle_program, .data = (void*)2 };
static const smenu_item_t _mm_item4 = { .label = "Verify", .handler = appop_handle_verify, .data = (void*)3 };
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

// Dynamic Menu Example (not used)
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

// Dynamic File Menu methods
static void _fm_destroy(const dynmenu_t* menu);
static const dynmenu_item_t* _fm_get_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
static const char* _fm_get_item_lbl(const dynmenu_t* menu, const dynmenu_item_t* item);
static const char* _fm_get_title(const dynmenu_t* menu);
static bool _fm_handle_item(const dynmenu_t* menu, const dynmenu_item_t* item);
static bool _fm_has_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype);
static void _fm_init(const dynmenu_t* menu);

// Dynamic File Menu item structure for linked-list
typedef struct fi_ll_ fill_t;
struct fi_ll_ {
    FILINFO finfo;
    dynmenu_item_t ditem;
    fill_t* prev;
    fill_t* next;
    bool last_file;
};
#define FILLP(x) ((fill_t*)(x))

// Dynamic Files Menu - The entries will be created dynamically as needed.
/*
dynmenu_item_t _fitem1 = { .get_label = _fm_get_item_lbl, .handler = _fm_handle_item, .data = (void*)0 };
dynmenu_item_content_t _fm_item1 = { .label = "File 1", .item = &_fitem1 };
*/
static dynmenu_t  _files_menu = { .type = MENU_DYNAMIC, .destroy = _fm_destroy, .init = _fm_init, .get_title = _fm_get_title, .get_item = _fm_get_item, .has_item = _fm_has_item, .data = (void*)0 };
static DIR _fm_dir;
static fill_t* _fill_root;
static msg_handler_fn _select_handler;
static char* _fselbuf;

static fill_t* _fm_fill_alloc() {
    fill_t* fill = malloc(sizeof(fill_t));
    if (!fill) {
        board_panic("!!! appmenus could not malloc for fill !!!");
    }
    memset(fill, 0, sizeof(fill_t));
    fill->ditem.data = (void*)fill;
    fill->ditem.get_label = _fm_get_item_lbl;
    fill->ditem.handler = _fm_handle_item;
    return (fill);
}

static fill_t* _fill_find(const dynmenu_item_t* ditem) {
    // Find the fill_t that contains the ditem
    fill_t* fill = _fill_root;
    while (fill && &(fill->ditem) != ditem) {
        fill = fill->next;
    }
    return (fill);
}

static fill_t* _fill_next(const dynmenu_item_t* ditem) {
    fill_t* fill = NULL;
    if (!ditem) {
        // This indicates that they want the first item.
        fill = _fill_root;
        goto _finally;
    }
    // Else, see if there is one after this.
    //  first, just look at the chain
    fill_t* ifill = _fill_find(ditem);
    if (ifill && !ifill->last_file) {
        fill = ifill->next;
        if (!fill) {
            // at the end and not known to be the last, see if there are more
            FRESULT fr;
            fill_t* fill1 = _fm_fill_alloc();
            memcpy(&fill1->finfo, &ifill->finfo, sizeof(FILINFO));
            fr = f_findnext(&_fm_dir, &fill1->finfo);
            do {
                if (fr == FR_NO_FILE || (fr == FR_OK && !fill1->finfo.fname[0])) {
                    // No more files, mark that we are at the end
                    ifill->last_file = true;
                    free(fill1);
                    goto _finally;
                }
                if (!(fill1->finfo.fattrib & AM_DIR)) {
                    fill = fill1;
                    fill->prev = ifill;
                    ifill->next = fill;
                    goto _finally;
                }
                fr = f_findnext(&_fm_dir, &(fill1->finfo));
            }
            while (true);
        }
    }
_finally:
    return (fill);
}

static fill_t* _fill_prev(const dynmenu_item_t* ditem) {
    fill_t* fill = NULL;
    if (!ditem) {
        // There isn't a previous from none.
        goto _finally;
    }
    fill_t* ifill = _fill_find(ditem);
    if (ifill) {
        fill = ifill->prev;
    }
_finally:
    return (fill);
}

static void _fm_destroy(const dynmenu_t* menu) {
    // Free up all fill_t
    fill_t* fill = _fill_root;
    while (fill) {
        fill_t* nfill = fill->next;
        free(fill);
        fill = nfill;
    }
    _fill_root = NULL;
}

static const dynmenu_item_t* _fm_get_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype) {
    fill_t* fill = (reqtype == MI_NEXT ? _fill_next(ref_item) : _fill_prev(ref_item));
    const dynmenu_item_t* ditem = NULL;
    if (fill) {
        ditem = &fill->ditem;
    }
    return (ditem);
}

static const char* _fm_get_item_lbl(const dynmenu_t* menu, const dynmenu_item_t* item) {
    fill_t* fill = FILLP(item->data);
    return (fill->finfo.fname);
}

static const char* _fm_get_title(const dynmenu_t* menu) {
    return ("Files");
}

static bool _fm_handle_item(const dynmenu_t* menu, const dynmenu_item_t* item) {
    bool retval = true;
    if (_select_handler) {
        // Copy the filename into the caller's buffer and Exec the handler
        cmt_msg_t msg;
        fill_t* fill = FILLP(item->data);
        strcpynt(_fselbuf, fill->finfo.fname, FF_LFN_BUF);
        cmt_exec_init(&msg, _select_handler);
        msg.data.str = _fselbuf;
        postAPPMsg(&msg);
        retval = false; // Let them decide what they want to do with the menu.
    }
    return (retval);
}

static bool _fm_has_item(const dynmenu_t* menu, const dynmenu_item_t* ref_item, menu_itemreq_t reqtype) {
    fill_t* fill = (reqtype == MI_NEXT ? _fill_next(ref_item) : _fill_prev(ref_item));
    bool hasitem = false;
    if (fill) {
        hasitem = true;
    }
    return (hasitem);
}

static void _fm_init(const dynmenu_t* menu) {
    FRESULT fr;
    _fill_root = NULL;

    // Open the root dir
    char* dirpath = "/";
    fr = f_opendir(&_fm_dir, dirpath);
    if (fr != FR_OK) {
        const char* rerr = FRESULT_str(fr);
        shell_printferr("Cannon open dir: '%s'  FR: %u - %s\n", dirpath, (uint32_t)fr, rerr);
        goto _finally;
    }
    // Get the first file (if there is one)
    //
    // We allocate a fill_t object to hold it. If there are no files we set some
    // info in it to indicate that.
    //
    fill_t *fill1 = _fm_fill_alloc();
    _fill_root = fill1;
    fr = f_findfirst(&_fm_dir, &(fill1->finfo), dirpath, "*");
    do {
        if (fr == FR_NO_FILE || (fr == FR_OK && !fill1->finfo.fname[0])) {
            // There aren't any files, so set special values so we'll know later.
            strcpy(fill1->finfo.fname, "No Files");
            strcpy(fill1->finfo.altname, "No Files");
            fill1->finfo.fsize = 0;
            fill1->finfo.fdate = 0;
            fill1->finfo.ftime = 0;
            fill1->last_file = true;
            goto _finally;
        }
        if (!(fill1->finfo.fattrib & AM_DIR)) {
            goto _finally;
        }
        fr = f_findnext(&_fm_dir, &(fill1->finfo));
    } while(true);
_finally:
    return;
}

const dynmenu_t* appmenu_files_select(msg_handler_fn select_handler, char* fselbuf) {
    // Hang on to the handler and buffer.
    _select_handler = select_handler;
    _fselbuf = fselbuf;
    // Return the menu
    return (&_files_menu);
}


void appmenu_modinit() {
    if (_initialized) {
        board_panic("!!! appmenu_modinit: Called more than once !!!");
    }
    _initialized = true;
    // Nothing special needed.
}

