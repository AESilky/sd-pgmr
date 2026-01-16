/**
 * Menu Manager.
 *
 * Manages a User-Interface menuing system
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "menumgr.h"
#include "appmenus.h"

#include "board.h"      // For `board_panic` and 'error'/'warn'/'info'_printf functions
#include "cmt.h"        // For message handling, sleep, etc.
#include "msgpost.h"    // For message posting
#include "display.h"    // For output
#include "include/util.h"

#include <stdlib.h> // malloc
#include <stdio.h>
#include <string.h> // memset

#define NONE_NDX (-1)
#define SPCH ' '    // SPACE character
#define CHAR_DOWN_ARROW 0x1A
#define CHAR_UP_ARROW 0x1B

#define DITEMP(c) ((const dynmenu_item_t*)(c))
#define DMENUP(c) ((const dynmenu_t*)(c))
#define MNUP(c) ((const mnu_t*)(c))
#define MITEMP(c) ((const mnu_item_t*)(c))
#define SITEMP(c) ((const smenu_item_t*)(c))
#define SMENUP(c) ((const smenu_t*)(c))

#define _TMP_BUF_LEN 50

// ====================================================================
// Data Types
// ====================================================================

typedef struct mnu_item_base_s_ {
    void* data; // Can be anything of meaning to the item functions
} mnu_item_t;
typedef struct mnu_menu_base_s_ {
    menu_type_t type; // Type must be 1st and set to 'MENU_DYNAMIC' or 'MENU_STATIC'
    void* data; // Can be anything of meaning to the item functions
} mnu_t;


typedef struct menu_stack_s_ menu_stack_t;

struct menu_stack_s_ {
    menu_stack_t* prev;
    menu_stack_t* next;
    const mnu_t* menu;
    int item_cnt; // Number of items for a static menu
    const mnu_item_t* d1_item;
    const mnu_item_t* sel_item;
    bool has_prior;
    bool has_later;
};

// ====================================================================
// Data
// ====================================================================

static volatile bool _initialized;

static volatile bool _dlg_active;
static volatile bool _menu_active;
static const display_info_t* _disp_info;

static const mnu_t* _main_menu;
static menu_stack_t* _menus_root;
static menu_stack_t* _menus_head;

static int _item_current_ndx;
static int _items_display_max;
static int _items_displayed_cnt;
static const mnu_item_t** _items_on_display;

// Dialog stuff
static struct dlg_cntx_* _dlg_root;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================

static void _display_item(menu_type_t type, int item_ndx, unsigned short row, bool inverse);
static void _display_updn_ind();
static void _dlg_update(dlg_ctx_t* dlg);
static void _item_current_next();
static void _item_current_prior();
static void _item_make_current(const mnu_item_t* item);
static void _item_select(const mnu_item_t* item);
static const mnu_t* _pop_menu();
static bool _pop_to_menu(const mnu_t* menu);
static void _push_menu(const mnu_t* menu);
static void _show_current_dynmenu(bool new);
static void _show_current_smenu(bool new);


// ====================================================================
// Run-After/Delay/Sleep Methods
// ====================================================================

/**
 * @brief Called after delay.
 *
 * This has been delayed.
 *
 * @param data Nothing important (can be pointer to anything needed)
 */
static void _delay_action(void* data) {
}


// ====================================================================
// Message Handler Methods
// ====================================================================

/**
 * @brief Handle dialog timeout.
 *
 * Occurs when a dialog is showing and it has a timeout set.
 * In the case of timeout, the dialog 'cancel' handler should be called.
 *
 * @param msg The void* is the dialog context pointer.
 */
static void _handle_dialog_timeout(cmt_msg_t* msg) {
    dlg_ctx_t* dlg = (dlg_ctx_t*)(msg->data.ptr);

    *dlg->_val = dlg->_valorig; // Put the original value back
    cmt_msg_t msg2;
    cmt_exec_init(&msg2, dlg->_on_cancel);
    postAPPMsg(&msg2);
}

/**
 * @brief Handle our dynmenu_enter(const dynmenu_t* menu) method.
 *
 * Action is performed in a dedicated handler to allow entering a menu from
 * an Item handler method.
 *
 * @param msg The void* is the dynmenu pointer.
 */
static void _handle_dmenu_enter(cmt_msg_t* msg) {
    const dynmenu_t* menu = DMENUP(msg->data.ptr);

    if (!_pop_to_menu(MNUP(menu))) {
        // The menu is new to us... Init it.
        menu->init(menu);
        _push_menu(MNUP(menu));
    }
    _show_current_dynmenu(true);
    _menu_active = true;
}

/**
 * @brief Handle our smenu_enter(const smenu_t* menu) method.
 *
 * Action is performed in a dedicated handler to allow entering a menu from
 * an Item handler method.
 *
 * @param msg The void* is the smenu pointer.
 */
static void _handle_smenu_enter(cmt_msg_t* msg) {
    const smenu_t* menu = SMENUP(msg->data.ptr);

    if (!_pop_to_menu(MNUP(menu))) {
        _push_menu(MNUP(menu));
    }
    _show_current_smenu(true); // Show as 'new'
    _menu_active = true;
}

/**
 * @brief Handle our Housekeeping tasks. This is triggered every ~16ms.
 *
 * For reference, 625 times is 10 seconds, or 62.5Hz.
 *
 * @param msg Nothing important in the message.
 */
static void _handle_housekeeping(cmt_msg_t* msg) {
    static uint cnt = 0;

    cnt++;
}

static void _handle_rotary_change(cmt_msg_t* msg) {
    static uint32_t _hrc_last_ts = 0;
    if (_menu_active || _dlg_active) {
        if (msg->t > (_hrc_last_ts + 200)) {
            _hrc_last_ts = msg->t;
            int16_t delta = msg->data.value16;
            if (delta != 0) {
                //
                // Menu or Dialog
                //
                if (_menu_active) {
                    //
                    // If delta is negative, move current menu item down, else up.
                    if (delta > 0) {
                        _item_current_prior();
                    }
                    else if (delta < 0) {
                        _item_current_next();
                    }
                }
                else if (_dlg_active) {
                    //
                    // Get the active dialog
                    dlg_ctx_t* dlg = _dlg_root;
                    while (dlg && !dlg->_focus) {
                        dlg = dlg->_next;
                    }
                    if (!dlg) {
                        // No dialog has focus
                        goto _finally;
                    }
                    bool chg = false;
                    if (delta > 0 && *(dlg->_val) > 0) {
                        *(dlg->_val) -= 1;
                        chg = true;
                    }
                    else if (delta < 0 && *(dlg->_val) < dlg->_max) {
                        *(dlg->_val) += 1;
                        chg = true;
                    }
                    if (chg) {
                        _dlg_update(dlg);
                    }
                }
            }
        }
    }
_finally:
    return;
}

static void _handle_switch_action(cmt_msg_t* msg) {
    // Handle switch actions for the menuing
    //
    if (_menu_active || _dlg_active) {
        switch_id_t sw = msg->data.sw_action.switch_id;
        bool pressed = msg->data.sw_action.pressed;
        bool longpress = msg->data.sw_action.longpress;

        if (sw == SW_ATTNCMD) {
            // Command/Attention Switch
            if (longpress) {
                if (_menu_active) {
                    // Move to the Main Menu
                    menu_display_main();
                }
            }
            else if (!pressed) {
                if (_menu_active) {
                    // Move up one item if possible
                    if (_menus_head->prev) {
                        _pop_menu();
                        menu_display_current();
                    }
                }
                else if (_dlg_active) {
                    // Cancel the current dialog
                    //
                    // Get the active dialog
                    dlg_ctx_t* dlg = _dlg_root;
                    while (dlg && !dlg->_focus) {
                        dlg = dlg->_next;
                    }
                    if (!dlg) {
                        // No dialog has focus
                        goto _finally;
                    }
                    scheduled_msg_cancel(MSG_DLG_TIMEOUT);
                    *dlg->_val = dlg->_valorig; // Put the original value back
                    cmt_msg_t msg;
                    cmt_exec_init(&msg, dlg->_on_cancel);
                    postAPPMsg(&msg);
                }
            }
        }
        else {
            // Rotary PB Switch
            if (!pressed) {
                if (_menu_active) {
                    // Select the item on release
                    _item_select(_items_on_display[_item_current_ndx]);
                }
                else if (_dlg_active) {
                    //
                    // Get the active dialog
                    dlg_ctx_t* dlg = _dlg_root;
                    while (dlg && !dlg->_focus) {
                        dlg = dlg->_next;
                    }
                    if (!dlg) {
                        // No dialog has focus
                        goto _finally;
                    }
                    scheduled_msg_cancel(MSG_DLG_TIMEOUT);
                    cmt_msg_t msg;
                    cmt_exec_init(&msg, dlg->_on_enter);
                    postAPPMsg(&msg);
                }
            }
        }
        msg->abort = true; // We handled this, so don't have others get it.
    }
_finally:
}


// ====================================================================
// Local/Private Methods
// ====================================================================

static int _display_ditems(menu_stack_t* active, const dynmenu_item_t* item1) {
    // Display items until we run out or hit the end of the display
    int row = 1;
    int items_displayed = 0;
    _items_on_display[0] = NULL;
    _items_on_display[1] = NULL;
    const dynmenu_t* menu = DMENUP(active->menu);
    const dynmenu_item_t* item = (item1 ? item1 : menu->get_item(menu, NULL, MI_NEXT));
    while (row < _disp_info->rows) {
        // Get an item
        _items_on_display[items_displayed] = MITEMP(item);
        if (!item) {
            break; // No more items
        }
        _display_item(menu->type, items_displayed, row, false);
        items_displayed++;
        row++;
        item = menu->get_item(menu, item, MI_NEXT);
    }
    _items_displayed_cnt = items_displayed;
    _display_updn_ind();
    return (items_displayed);
}

static void _display_item(menu_type_t type, int item_ndx, unsigned short row, bool inverse) {
    const char* label;
    if (type == MENU_DYNAMIC) {
        const dynmenu_t* menu = DMENUP(_menus_head->menu);
        const dynmenu_item_t* item = DITEMP(_items_on_display[item_ndx]);
        label = item->get_label(menu, item);
    }
    else {
        const smenu_item_t* item = SITEMP(_items_on_display[item_ndx]);
        label = item->label;
    }
    uint8_t attr = (inverse ? DISP_CHAR_INVERT_BIT : 0);
    bool instr = true;
    for (int i = 0; i < _disp_info->cols; i++) {
        instr = instr && label[i];
        uint8_t c = (instr ? label[i] : SPCH) | attr;
        display_char(row, i, c, false, false);
    }
    display_paint();
}

static int _display_sitems(menu_stack_t* active, const smenu_item_t* d1_item) {
    // Display items until we run out or hit the end of the display
    int items_displayed = 0;
    _items_on_display[0] = NULL;
    _items_on_display[1] = NULL;
    const smenu_t* menu = SMENUP(active->menu);
    int itemndx = 0;
    if (d1_item) {
        // A first item was specified, start with that item...
        for (; itemndx < active->item_cnt; itemndx++) {
            if (menu->items[itemndx] == d1_item) {
                break;
            }
        }
    }
    while (items_displayed < _disp_info->rows - 1) {
        // Get an item
        const smenu_item_t* item = NULL;
        if (itemndx < active->item_cnt) {
            item = menu->items[itemndx];
        }
        _items_on_display[items_displayed] = MITEMP(item);
        if (!item) {
            break; // No more items
        }
        _display_item(menu->type, items_displayed, items_displayed + 1, false);
        itemndx++;
        items_displayed++;
    }
    _items_displayed_cnt = items_displayed;
    _display_updn_ind();

    return (items_displayed);
}

static void _display_title(const char* title) {
    // Print the title centered and the whole row underlined.
    display_clear(true);
    int len = min(strlen(title), _disp_info->cols); // For centering
    int pad = constrain((_disp_info->cols - len), 0, _disp_info->cols);
    int p1 = pad / 2;
    unsigned short int col = 0;
    unsigned short int i;
    for (i = 0; i < p1; i++) {
        display_char(0, col++, SPCH, true, NoPaint); // underline
    }
    for (i = 0; i < len; i++) {
        display_char(0, col++, title[i], true, NoPaint);
    }
    while (col < _disp_info->cols) {
        display_char(0, col++, SPCH, true, NoPaint); // underline
    }
    display_paint();
}

/* Display the UP/DOWN arrows if needed. */
static void _display_updn_ind() {
    menu_type_t type = _menus_head->menu->type;
    int lastrow = _disp_info->rows - 1;
    if (type == MENU_DYNAMIC) {
        const dynmenu_t* dmenu = DMENUP(_menus_head->menu);
        const dynmenu_item_t* ditem = DITEMP(_items_on_display[0]);
        // Do the first row
        if (dmenu->has_item(dmenu, ditem, MI_PRIOR)) {
            // Need an up arrow. Should it be inverse?
            uint8_t attr = (_item_current_ndx == 0 ? DISP_CHAR_INVERT_BIT : 0);
            display_char(1, _disp_info->cols - 1, CHAR_UP_ARROW | attr, false, Paint);
        }
        // Do the last row
        ditem = DITEMP(_items_on_display[_items_displayed_cnt - 1]);
        if (dmenu->has_item(dmenu, ditem, MI_NEXT)) {
            // Need a down arrow. Should it be inverse?
            uint8_t attr = (_item_current_ndx == lastrow - 1 ? DISP_CHAR_INVERT_BIT : 0);
            display_char(lastrow, _disp_info->cols - 1, CHAR_DOWN_ARROW | attr, false, Paint);
        }
    }
    else {
        const smenu_t* smenu = SMENUP(_menus_head->menu);
        const smenu_item_t* sitem = SITEMP(_items_on_display[0]);
        // Do the first row
        if (sitem != smenu->items[0]) {
            // Need an up arrow. Should it be inverse?
            uint8_t attr = (_item_current_ndx == 0 ? DISP_CHAR_INVERT_BIT : 0);
            display_char(1, _disp_info->cols - 1, CHAR_UP_ARROW | attr, false, Paint);
        }
        // Do the last row
        sitem = SITEMP(_items_on_display[_items_displayed_cnt - 1]);
        //  Find its index
        int ndx = 0;
        while (sitem != smenu->items[ndx]) {
            ndx++;
        }
        if (ndx != _menus_head->item_cnt - 1) {
            // Need a down arrow. Should it be inverse?
            uint8_t attr = (_item_current_ndx == lastrow - 1 ? DISP_CHAR_INVERT_BIT : 0);
            display_char(lastrow, _disp_info->cols - 1, CHAR_DOWN_ARROW | attr, false, Paint);
        }
    }
}

static dlg_ctx_t* _dlg_alloc() {
    size_t sz = sizeof(dlg_ctx_t);
    dlg_ctx_t* cntx = (dlg_ctx_t*)malloc(sz);
    if (cntx) {
        memset(cntx, 0, sz);
        cntx->_next = _dlg_root;
        _dlg_root = cntx;
    }
    return (cntx);
}

static void _dlg_focus(dlg_ctx_t* dlg) {
    // Give this one focus and remove focus from any others
    //
    // Get the dialog with focus, if there is one.
    dlg_ctx_t* df = _dlg_root;
    while (df && !df->_focus) {
        df = df->_next;
    }
    if (df) {
        df->_focus = false;
        // cancel any timeout
        int32_t msl = scheduled_msg_cancel(MSG_DLG_TIMEOUT);
        df->_toms = msl;
    }
    // See if it has a timeout. If so, start it.
    if (dlg->_toms) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_DLG_TIMEOUT);
        msg.data.ptr = MDPTR(dlg);
        schedule_msg_in_ms(dlg->_toms, &msg);
    }
    dlg->_focus = true;
    _dlg_active = true;
}

static void _dlg_update(dlg_ctx_t* dlg) {
    // ZZZ: Will need to handle different types...
    char buf[_TMP_BUF_LEN];
    snprintf(buf, _TMP_BUF_LEN, "%*u", dlg->_maxdgts, *(dlg->_val));
    display_string(dlg->_row, dlg->_col, buf, true, false, Paint);
}

/* Move the hilite to the next item (if possible) */
static void _item_current_next() {
    // See if it is easy to advance
    if (_item_current_ndx <= _items_displayed_cnt - 2) {
        const mnu_item_t* item = _items_on_display[_item_current_ndx + 1];
        _item_make_current(item);
        return;
    }
    // The current item is the last one displayed, do additional checks.
    if (_items_displayed_cnt < _items_display_max) {
        // There are fewer items than the display can hold and we are at the end.
        return;
    }
    // Okay, real work is needed...
    menu_type_t type = _menus_head->menu->type;
    if (type == MENU_DYNAMIC) {
        const dynmenu_t* dmenu = DMENUP(_menus_head->menu);
        const dynmenu_item_t* ditem = DITEMP(_items_on_display[_item_current_ndx]);
        // See if there is a next item
        if (dmenu->has_item(dmenu, ditem, MI_NEXT)) {
            // There is another item, so repaint the menu one down and make last item current.
            _display_ditems(_menus_head, DITEMP(_items_on_display[1]));
            _item_make_current(_items_on_display[_items_displayed_cnt - 1]);
        }
    }
    else {
        // Find where the current item is in the list of items
        const smenu_t* smenu = SMENUP(_menus_head->menu);
        const smenu_item_t* sitem = SITEMP(_items_on_display[_item_current_ndx]);
        int item_num = 0;
        while (sitem != smenu->items[item_num]) {
            item_num++;
        }
        if (item_num < _menus_head->item_cnt - 1) {
            // There is another item, so repaint the menu one down and make last item current.
            _display_sitems(_menus_head, SITEMP(_items_on_display[1]));
            _item_make_current(_items_on_display[_items_displayed_cnt - 1]);
        }
    }
}

/* Move the hilite to the prior item (if possible) */
static void _item_current_prior() {
    // See if it is easy to retard
    if (_item_current_ndx > 0) {
        const mnu_item_t* item = _items_on_display[_item_current_ndx - 1];
        _item_make_current(item);
        return;
    }
    // The current item is the first one displayed, do additional checks.
    menu_type_t type = _menus_head->menu->type;
    if (type == MENU_DYNAMIC) {
        const dynmenu_t* dmenu = DMENUP(_menus_head->menu);
        const dynmenu_item_t* ditem = DITEMP(_items_on_display[_item_current_ndx]);
        // See if there is a prior item
        const dynmenu_item_t* dprior = dmenu->get_item(dmenu, ditem, MI_PRIOR);
        if (dprior) {
            // There is a prior item, so repaint the menu one up and make first item current.
            _display_ditems(_menus_head, dprior);
            _item_make_current(MITEMP(dprior));
        }
    }
    else {
        // Find where the current item is in the list of items
        const smenu_t* smenu = SMENUP(_menus_head->menu);
        const smenu_item_t* sitem = SITEMP(_items_on_display[_item_current_ndx]);
        int item_num = 0;
        while (sitem != smenu->items[item_num]) {
            item_num++;
        }
        if (item_num > 0) {
            // There is a prior item, so repaint the menu one up and make first item current.
            _display_sitems(_menus_head, smenu->items[item_num - 1]);
            _item_make_current(_items_on_display[0]);
        }
    }
}

static void _item_make_current(const mnu_item_t* item) {
    menu_type_t type = _menus_head->menu->type;
    int itemndx = 0;
    if (item) {
        // Find which row it's on
        while (itemndx < _items_displayed_cnt) {
            if (_items_on_display[itemndx] == item) {
                break;
            }
            itemndx++;
        }
    }
    if (_item_current_ndx != NONE_NDX && itemndx != _item_current_ndx) {
        // Paint the current 'current' item normally
        _display_item(type, _item_current_ndx, _item_current_ndx + 1, false);
    }
    _item_current_ndx = itemndx;
    _display_item(type, _item_current_ndx, _item_current_ndx + 1, true); // Print the item reversed
    _display_updn_ind();

    return;
}

static void _item_select(const mnu_item_t* itemsel) {
    menu_type_t type = _menus_head->menu->type;
    if (type == MENU_DYNAMIC) {
        const dynmenu_item_t* item = DITEMP(itemsel);
        if (item->handler) {
            const dynmenu_t* menu = DMENUP(_menus_root->menu);
            _menu_active = false; // Deactivate the menu so the handler has the input.
            _menu_active = item->handler(menu, item);  // Handler returns true to (re)activate this menu.
        }
    }
    else {
        const smenu_item_t* item = SITEMP(itemsel);
        if (item->handler) {
            const smenu_t* menu = SMENUP(_menus_root->menu);
            _menu_active = false; // Deactivate the menu so the handler has the input.
            _menu_active = item->handler(menu, item);  // Handler returns true to (re)activate this menu.
        }
    }
}

static const mnu_t* _pop_menu() {
    menu_stack_t* current = _menus_head;
    // pop current off
    if (current && current->prev) {
        _menus_head = current->prev;
        if (current->menu->type == MENU_DYNAMIC) {
            // Call 'destroy' on Dynamic Menus
            const dynmenu_t* dmenu = DMENUP(current->menu);
            dmenu->destroy(dmenu);
        }
        free(current);
    }
    const mnu_t* menu = (_menus_head ? _menus_head->menu : NULL);

    return (menu);
}

static bool _pop_to_menu(const mnu_t* menu) {
    // See if the menu exists in the stack
    menu_stack_t* current = _menus_head;
    while(current) {
        if (current->menu == menu) {
            break;
        }
        current = current->prev;
    }
    if (!current) {
        return (false);
    }
    // The menu is in the stack, so pop menus until found.
    const mnu_t* cm;
    do {
        cm = _pop_menu();
    } while(cm && cm != menu);
    return (true);
}

static void _push_menu(const mnu_t* menu) {
    size_t ms = sizeof(menu_stack_t);
    menu_stack_t* si = (menu_stack_t *)malloc(ms);
    if (!si) {
        board_panic("!menumgr: malloc failed for _push_menu");
    }
    memset(si, 0, ms);
    si->menu = menu;
    // If this is a static menu, count the items
    if (menu->type == MENU_STATIC) {
        si->item_cnt = 0;
        const smenu_t* smenu = (const smenu_t*)menu;
        const smenu_item_t** itemv = smenu->items;
        while (*itemv) {
            si->item_cnt++;
            itemv++;
        }
        if (si->item_cnt > (_disp_info->rows - 1)) {
            si->has_later = true;
        }
    }
    else {
        si->item_cnt = -1;
    }
    if (_menus_root == NULL) {
        _menus_root = si;
        _menus_head = si;
        // This is the first menu, set it as the 'MAIN Menu'.
        _main_menu = menu;
    }
    else {
        si->prev = _menus_head;
        _menus_head = si;
    }
}

static void _show_current_dynmenu(bool new) {
    menu_stack_t* stkmenu = _menus_head; // Get the menu at the top of the stack
    if (stkmenu->menu->type != MENU_DYNAMIC) {
        const char* title = SMENUP(stkmenu->menu)->title;
        board_panic("_show_current_dynmenu - called to process STATIC menu: '%s'\n", title);
    }
    const dynmenu_t* dmenu = (const dynmenu_t*)stkmenu->menu;
    const char* title = dmenu->get_title(dmenu);
    _display_title(title);
    _display_ditems(stkmenu, NULL);
    if (_items_displayed_cnt > 0) {
        _item_make_current(MITEMP(0));
    }
}

static void _show_current_smenu(bool new) {
    menu_stack_t* stkmenu = _menus_head; // Get the menu at the top of the stack
    if (stkmenu->menu->type != MENU_STATIC) {
        const dynmenu_t* dmenu = DMENUP(stkmenu->menu);
        const char* title = dmenu->get_title(dmenu);
        board_panic("_show_current_smenu - called to process DYNAMIC menu: '%s'\n", title);
    }
    const smenu_t* smenu = SMENUP(stkmenu->menu);
    display_clear(true);
    unsigned short int row = 0;
    unsigned short int col = 0;
    const char* title = smenu->title;
    int len = min(strlen(title), _disp_info->cols); // For centering
    int pad = constrain((_disp_info->cols - len), 0, _disp_info->cols);
    int p1 = pad / 2;
    unsigned short int i;
    // Print the title centered and the whole row underlined.
    for (i = 0; i < p1; i++) {
        display_char(row, col++, SPCH, true, NoPaint); // underline
    }
    for (i = 0; i < len; i++) {
        display_char(row, col++, title[i], true, NoPaint);
    }
    while (col < _disp_info->cols) {
        display_char(row, col++, SPCH, true, NoPaint); // underline
    }
    display_paint();
    const smenu_item_t* d1_item = (new ? SITEMP(0) : SITEMP(stkmenu->sel_item));
    _items_displayed_cnt = _display_sitems(stkmenu, d1_item);
    if (_items_displayed_cnt > 0) {
        _item_make_current(MITEMP(d1_item));
    }
}


// ====================================================================
// Local Utility (non-menu/dialog) Methods
// ====================================================================

uint32_t _digits_count(uint32_t n) {
    if (n == 0) return 1; // Special case for zero
    int count = 0;
    while (n > 0) {
        n /= 10;
        count++;
    }
    return count;
}

// ====================================================================
// Public Methods
// ====================================================================

void dlg_dismiss(dlg_ctx_t* cntx) {
    if (cntx->_active) {
        _dlg_root = cntx->_next;
        cntx->_next = NULL;
        cntx->_active = false;
        cntx->_row = 0;
        cntx->_col = 0;
        cntx->_min = 0;
        cntx->_max = 0;
        cntx->_val = NULL;
        cntx->_on_enter = NULL;
        if (cntx->_focus) {
            _dlg_active = false;
        }
        free(cntx);
    }
}

dlg_ctx_t* dlg_wait_or_cancel(int32_t ms, msg_handler_fn on_enter, msg_handler_fn on_cancel) {
    dlg_ctx_t* cntx = _dlg_alloc();
    if (!cntx) {
        goto _finally;
    }
    //
    cntx->_toms = ms;
    cntx->_on_cancel = on_cancel;
    cntx->_on_enter = on_enter;
    cntx->_active = true;
    _dlg_focus(cntx);
_finally:
    return (cntx);
}

static void _handle_file_selected(cmt_msg_t* msg) {
    // The File Picker should be the active dialog
    dlg_ctx_t* dlg = _dlg_root;
    if (dlg) {
        cmt_msg_t msgexec;
        cmt_exec_init(&msgexec, dlg->_on_enter);
        msgexec.data.str = msg->data.str;
        postAPPMsg(&msgexec);
    }
}

dlg_ctx_t* dlg_file_pick(msg_handler_fn on_enter, msg_handler_fn on_cancel, char* fselbuf) {
    dlg_ctx_t* cntx = _dlg_alloc();
    if (!cntx) {
        goto _finally;
    }
    //
    cntx->_on_cancel = on_cancel;
    cntx->_on_enter = on_enter;
    cntx->_active = true;
    // Make the dialog active (no content)
    _dlg_focus(cntx);
    // A dynamic menu is used to display the files and select one.
    const dynmenu_t* file_menu = appmenu_files_select(_handle_file_selected, fselbuf);
    dynmenu_enter(file_menu);
_finally:
    return (cntx);
}

dlg_ctx_t* dlg_num_input(uint8_t row, uint8_t col, uint32_t* val, uint32_t min, uint32_t max, msg_handler_fn on_enter, msg_handler_fn on_cancel) {
    dlg_ctx_t* cntx = _dlg_alloc();
    if (!cntx) {
        goto _finally;
    }
    //
    cntx->_row = row;
    cntx->_col = col;
    cntx->_min = min;
    cntx->_max = max;
    cntx->_val = val;
    cntx->_valorig = *val; // keep original value for Cancel
    cntx->_on_cancel = on_cancel;
    cntx->_on_enter = on_enter;
    cntx->_active = true;
    // How many digits...
    cntx->_maxdgts = _digits_count(max);
    _dlg_update(cntx);
    _dlg_focus(cntx);
_finally:
    return (cntx);
}

void dynmenu_enter(const dynmenu_t* menu) {
    cmt_msg_t msg;
    cmt_exec_init(&msg, _handle_dmenu_enter);
    msg.data.ptr = (void*)menu;
    postAPPMsg(&msg);
}

void menu_deactivate() {
    _menu_active = false;
}

void menu_display_current() {
    if (_menus_head->menu->type == MENU_DYNAMIC) {
        _show_current_dynmenu(false);
    }
    else {
        _show_current_smenu(false);
    }
    _menu_active = true;
}

void menu_display_main() {
    if (_main_menu) {
        _pop_to_menu(_main_menu);
        menu_display_current();
    }
}

void smenu_enter(const smenu_t* menu) {
    cmt_msg_t msg;
    cmt_exec_init(&msg, _handle_smenu_enter);
    msg.data.ptr = (void*)menu;
    postAPPMsg(&msg);
}


// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================


void menumgr_modinit() {
    if (_initialized) {
        board_panic("!!! menumgr_modinit: Called more than once !!!");
    }
    _menus_head = _menus_root = NULL;
    _main_menu = NULL;

    // Get the info for the display to use while building screens (max items to show).
    _disp_info = display_info();
    // Allocate memory for a list of displayed menu item pointers.
    _items_display_max = _disp_info->rows - 1;
    _items_on_display = malloc((_items_display_max + 1) * sizeof(mnu_item_t*));
    if (!_items_on_display) {
        board_panic("! menumgr: Could not malloc for list of displayed items !");
    }
    memset(_items_on_display, 0, (_items_display_max + 1) * sizeof(mnu_item_t*));
    _item_current_ndx = NONE_NDX;

    // Add our message handlers
    cmt_msg_hdlr_add(MSG_DLG_TIMEOUT, _handle_dialog_timeout);
    cmt_msg_hdlr_add(MSG_ROTARY_CHG, _handle_rotary_change);
    cmt_msg_hdlr_add(MSG_SW_ACTION, _handle_switch_action);
}
