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
    const mnu_item_t* sel_item;
    bool has_prior;
    bool has_later;
};

// ====================================================================
// Data
// ====================================================================

static volatile bool _initialized;

static volatile bool _active_dlg;
static volatile bool _active_menu;
static const display_info_t* _disp_info;

static const mnu_t* app_main_menu;
static menu_stack_t* _menus_root;
static menu_stack_t* _menus_tail;

static int _item_current;
static int _item_current_row;
static int _items_display_max;
static int _items_displayed;
static int* _iod_indexes;
static const mnu_item_t** _items_on_display;

// Dialog stuff
static struct dlg_cntx_* _dlg_root;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================

static void _display_item(menu_type_t type, int item_ndx, unsigned short row, bool inverse);
static void _ditem_insrt_top(const dynmenu_t* dmenu, const dynmenu_item_t* rqst_item);
static void _dlg_update(dlg_ctx_t* dlg);
static void _item_make_current(const mnu_t* menu, int item_ndx);
static void _item_select(menu_type_t type, int item_ndx);
static const mnu_t* _pop_menu();
static bool _pop_to_menu(const mnu_t* menu);
static void _push_menu(const mnu_t* menu);
static void _show_current_dynmenu();
static void _show_current_smenu();


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
        _push_menu(MNUP(menu));
    }
    _show_current_dynmenu();
    _active_menu = true;
}

/**
 * @brief Handle our smenu_enter(const smenu_t* menu) method.
 *
 * Action is performed in a dedicated handler to allow entering a menu from
 * an Item handler method.
 *
 * @param msg The void* is the dynmenu pointer.
 */
static void _handle_smenu_enter(cmt_msg_t* msg) {
    const dynmenu_t* menu = DMENUP(msg->data.ptr);

    if (!_pop_to_menu(MNUP(menu))) {
        _push_menu(MNUP(menu));
    }
    _show_current_smenu();
    _active_menu = true;
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
    if (_active_menu || _active_dlg) {
        if (msg->t > (_hrc_last_ts + 200)) {
            _hrc_last_ts = msg->t;
            int16_t delta = msg->data.value16;
            if (delta != 0) {
                //
                // Menu or Dialog
                //
                if (_active_menu) {
                    //
                    // If delta is negative, move current menu item down, else up.
                    if (delta > 0) {
                        _item_make_current(_menus_tail->menu, _item_current - 1);
                    }
                    else if (delta < 0) {
                        _item_make_current(_menus_tail->menu, _item_current + 1);
                    }
                }
                else if (_active_dlg) {
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
    if (_active_menu || _active_dlg) {
        switch_id_t sw = msg->data.sw_action.switch_id;
        bool pressed = msg->data.sw_action.pressed;
        bool longpress = msg->data.sw_action.longpress;

        if (sw == SW_ATTNCMD) {
            // Command/Attention Switch
            if (longpress) {
                // Move to the Main Menu
                if (app_main_menu) {
                    _pop_to_menu(app_main_menu);
                    _show_current_dynmenu();
                }
            }
            else if (!pressed) {
                // Move up one item on release if possible
                if (_menus_tail->prev) {
                    _pop_menu();
                    _show_current_dynmenu();
                }
            }
        }
        else {
            // Rotary PB Switch
            if (!pressed) {
                if (_active_menu) {
                    // Select the item on release
                    menu_type_t type = _menus_tail->menu->type;
                    _item_select(type, _item_current);
                }
                else if (_active_dlg) {
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
                    cmt_msg_t msg;
                    cmt_exec_init(&msg, dlg->_on_change);
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

static int _display_ditems(menu_stack_t* stack, int prev_item) {
    // Display items until we run out or hit the end of the display
    int row = 1;
    int items_displayed = 0;
    _items_on_display[0] = NULL;
    _items_on_display[1] = NULL;
    const dynmenu_t* menu = DMENUP(stack->menu);
    const dynmenu_item_t* last = (prev_item < 0 ? NULL : DITEMP(_items_on_display[prev_item]));
    while (row < _disp_info->rows) {
        // Get an item
        const dynmenu_item_t* item = last;
        item = menu->get_item(menu, item, MI_NEXT);
        _items_on_display[items_displayed] = MITEMP(item);
        last = item;
        if (!item) {
            break; // No more items
        }
        _display_item(menu->type, items_displayed, row, false);
        items_displayed++;
        row++;
    }
    // If there are more items in the menu, put a DOWN-ARROW at the bottom right
    if (last && menu->has_item(menu, last, MI_NEXT)) {
        uint8_t da = CHAR_DOWN_ARROW;
        display_char(_disp_info->rows - 1, _disp_info->cols - 1, da, false, Paint);
    }

    return (items_displayed);
}

static void _display_item(menu_type_t type, int item_ndx, unsigned short row, bool inverse) {
    const char* label;
    if (type == MENU_DYNAMIC) {
        const dynmenu_t* menu = DMENUP(_menus_tail->menu);
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

static int _display_sitems(menu_stack_t* stack, int first_item) {
    // Display items until we run out or hit the end of the display
    int row = 1;
    int items_displayed = 0;
    _items_on_display[0] = NULL;
    _items_on_display[1] = NULL;
    const smenu_t* menu = SMENUP(stack->menu);
    const smenu_item_t* last = NULL;
    int itemndx = first_item;
    while (row < _disp_info->rows) {
        // Get an item
        const smenu_item_t* item = NULL;
        if (itemndx < stack->item_cnt) {
            item = menu->items[itemndx];
        }
        _items_on_display[items_displayed] = MITEMP(item);
        _iod_indexes[items_displayed] = (item ? itemndx : NONE_NDX);
        last = item;
        if (!item) {
            break; // No more items
        }
        _display_item(menu->type, items_displayed, row, false);
        itemndx++;
        items_displayed++;
        row++;
    }
    // If there are more items in the menu, put a DOWN-ARROW at the bottom right
    if (last && itemndx < stack->item_cnt) {
        uint8_t da = CHAR_DOWN_ARROW;
        display_char(_disp_info->rows - 1, _disp_info->cols - 1, da, false, Paint);
    }

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

static void _ditem_insrt_top(const dynmenu_t* dmenu, const dynmenu_item_t* rqst_item) {
    printf("\nZZZ - Insert a Dynamic menu item at the top\n");
}

static void _dlg_update(dlg_ctx_t* dlg) {
    // ZZZ: Will need to handle different types...
    char buf[_TMP_BUF_LEN];
    snprintf(buf, _TMP_BUF_LEN, "%*u", dlg->_maxdgts, *(dlg->_val));
    display_string(dlg->_row, dlg->_col, buf, true, false, Paint);
}

static void _item_make_current(const mnu_t* menu, int item_ndx) {
    menu_type_t type = menu->type;
    // See if the indicated item exists
    if (item_ndx < 0) {
        // If Static, select item 0. If Dynamic, see if there is an item before the current 0.
        if (type == MENU_STATIC) {
            item_ndx = 0;
        }
        else {
            const dynmenu_t* dmenu = DMENUP(menu);
            const dynmenu_item_t* ditem = DITEMP(_items_on_display[0]);
            const dynmenu_item_t* rqst_item = dmenu->get_item(dmenu, ditem, MI_PRIOR);
            if (rqst_item) {
                _ditem_insrt_top(dmenu, rqst_item);
                item_ndx = 0;
            }
        }
    }
    else if (item_ndx > _iod_indexes[_items_displayed-1]) {
        // If Static, see if it is within bounds. If Dynamic, see if that is an additional item.
        if (type == MENU_STATIC) {
            if (item_ndx < _menus_tail->item_cnt) {
                // Find the first index to display
                int i = (_items_displayed - 1);
                while (i >= 0 && _iod_indexes[i] > item_ndx) {
                    i--;
                }
                // Move back, up to _items_display_max indexes
                i = constrain(i-3, 0, (_items_displayed - 1));
                int ndx = _iod_indexes[i];
                _display_sitems(_menus_root, ndx);
                _item_current_row = 0;
                while (_item_current != _iod_indexes[_item_current_row] && _item_current_row < (_items_display_max - 2)) {
                    _item_current_row++;
                }
                _item_current_row++;
            }
        }
    }
    if (item_ndx == _item_current) {
        goto _finally;
    }
    int last_item = _item_current;
    if (_item_current != NONE_NDX) {
        // Paint the current item normally
        _display_item(type, _item_current, _item_current_row, false);
    }
    _item_current = item_ndx;
    while (_item_current != _iod_indexes[_item_current_row] && _item_current_row < (_items_display_max - 2)) {
        _item_current_row++;
    }
    _item_current_row++;
    _display_item(type, _item_current, _item_current_row, true); // Print the item reversed
    // If painting the last item on display, repaint arrow if needed.
    int last_item_row = _disp_info->rows - 2;
    if (item_ndx == last_item_row || last_item == last_item_row) {
        bool need_arrow = false;
        if (type == MENU_DYNAMIC) {
            const dynmenu_t* dmenu = DMENUP(menu);
            const dynmenu_item_t* ditem = DITEMP(_items_on_display[last_item_row]);
            if (dmenu->has_item(dmenu, ditem, MI_NEXT)) {
                need_arrow = true;
            }
        }
        else {
            if (item_ndx < _menus_tail->item_cnt - 1) {
                need_arrow = true;
            }
        }
        if (need_arrow) {
            uint8_t attr = (last_item == last_item_row ? 0 : DISP_CHAR_INVERT_BIT);
            uint8_t da = (CHAR_DOWN_ARROW | attr);
            display_char(_disp_info->rows - 1, _disp_info->cols - 1, da, false, Paint);
        }
    }
_finally:
    return;
}

static void _item_select(menu_type_t type, int item_ndx) {
    if (type == MENU_DYNAMIC) {
        const dynmenu_item_t* item = DITEMP(_items_on_display[item_ndx]);
        if (item->handler) {
            const dynmenu_t* menu = DMENUP(_menus_root->menu);
            _active_menu = false; // Deactivate the menu so the handler has the input.
            _active_menu = item->handler(menu, item);  // Handler returns true to (re)activate this menu.
        }
    }
    else {
        const smenu_item_t* item = SITEMP(_items_on_display[item_ndx]);
        if (item->handler) {
            const smenu_t* menu = SMENUP(_menus_root->menu);
            _active_menu = false; // Deactivate the menu so the handler has the input.
            _active_menu = item->handler(menu, item);  // Handler returns true to (re)activate this menu.
        }
    }
}

static const mnu_t* _pop_menu() {
    menu_stack_t* current = _menus_tail;
    // pop current off
    if (current) {
        _menus_tail = current->prev;
        free(current);
    }
    const mnu_t* menu = (_menus_tail ? _menus_tail->menu : NULL);

    return (menu);
}

static bool _pop_to_menu(const mnu_t* menu) {
    // See if the menu exists in the stack
    menu_stack_t* current = _menus_tail;
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
    } while(cm != menu);
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
        _menus_tail = si;
        // This is the first menu, set it as the root.
        app_main_menu = menu;
    }
    else {
        si->prev = _menus_tail;
        _menus_tail = si;
    }
}

static void _show_current_dynmenu() {
    menu_stack_t* stkmenu = _menus_tail; // Get the item at the tail of the stack
    if (stkmenu->menu->type != MENU_DYNAMIC) {
        const char* title = SMENUP(stkmenu->menu)->title;
        board_panic("_show_current_dynmenu - called to process STATIC menu: '%s'\n", title);
    }
    const dynmenu_t* dmenu = (const dynmenu_t*)stkmenu->menu;
    const char* title = dmenu->get_title(dmenu);
    _display_title(title);
    _items_displayed = _display_ditems(stkmenu, NONE_NDX);
    if (_items_displayed > 0) {
        _item_make_current(MNUP(dmenu), 0);
    }
}

static void _show_current_smenu() {
    menu_stack_t* stkmenu = _menus_tail; // Get the item at the tail of the stack
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
    _items_displayed = _display_sitems(stkmenu, 0);
    if (_items_displayed > 0) {
        _item_make_current(MENU_STATIC, 0);
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
        cntx->_on_change = NULL;
        if (cntx->_focus) {
            _active_dlg = false;
        }
        free(cntx);
    }
}

dlg_ctx_t* dlg_num_input(uint8_t row, uint8_t col, uint32_t* val, uint32_t min, uint32_t max, msg_handler_fn on_change) {
    dlg_ctx_t* cntx = (dlg_ctx_t*)malloc(sizeof(dlg_ctx_t));
    if (!cntx) {
        goto _finally;
    }
    cntx->_active = true;
    cntx->_next = _dlg_root;
    _dlg_root = cntx;
    //
    cntx->_row = row;
    cntx->_col = col;
    cntx->_min = min;
    cntx->_max = max;
    cntx->_val = val;
    cntx->_valorig = *val; // keep original value for Cancel
    cntx->_on_change = on_change;
    // How many digits...
    cntx->_maxdgts = _digits_count(max);
    _dlg_update(cntx);
    // Give this one focus and remove focus from any others
    cntx->_focus = true;
    dlg_ctx_t* dlgs = cntx->_next;
    while (dlgs) {
        dlgs->_focus = false;
        dlgs = dlgs->_next;
    };
    _active_dlg = true;
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
    _active_menu = false;
}

void menu_display_current() {
    const mnu_t* menu = _menus_tail->menu;
    if (menu->type == MENU_DYNAMIC) {
        _show_current_dynmenu();
    }
    else {
        _show_current_smenu();
    }
    _active_menu = true;
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


void menumgr_minit() {
    if (_initialized) {
        board_panic("!!! menumgr_minit: Called more than once !!!");
    }
    _menus_tail = _menus_root = NULL;
    app_main_menu = NULL;

    // Get the info for the display to use while building screens.
    _disp_info = display_info();
    // Allocate memory for a list of displayed menu item pointers.
    _items_display_max = _disp_info->rows - 1;
    _items_on_display = malloc((_items_display_max + 1) * sizeof(mnu_item_t*));
    _iod_indexes = malloc(_items_display_max * sizeof(int));
    if (!_items_on_display || !_iod_indexes) {
        board_panic("! menumgr: Could not malloc for list of displayed items !");
    }
    memset(_items_on_display, 0, (_items_display_max + 1) * sizeof(mnu_item_t*));
    for (int i = 0; i < _items_display_max; i++) {
        _iod_indexes[i] = NONE_NDX;
    }
    _item_current = NONE_NDX;

    // Add our message handlers
    cmt_msg_hdlr_add(MSG_ROTARY_CHG, _handle_rotary_change);
    cmt_msg_hdlr_add(MSG_SW_ACTION, _handle_switch_action);
}
