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

#include "board.h" // For `board_panic` and 'error'/'warn'/'info'_printf functions
#include "cmt.h" // For message handling
#include "display.h" // For output
#include "include/util.h"

#include <stdlib.h> // malloc
#include <string.h> // memset

#define NONE_NDX (-1)
#define SPCH ' '    // SPACE character

// ====================================================================
// Data Types
// ====================================================================

typedef struct stack_menu_s_ stack_menu_t;

struct stack_menu_s_ {
    stack_menu_t* prev;
    stack_menu_t* next;
    const menu_t* menu;
    const menu_item_t* sel_item;
};

// ====================================================================
// Data
// ====================================================================

static volatile bool _initialized;

static bool _active;
static display_info_t _disp_info;

static const menu_t* _main_menu;
static stack_menu_t* _menus_root;
static stack_menu_t* _menus_tail;

static int _item_current;
static int _items_displayed;
static const menu_item_t** _items_on_display;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================

static void _item_make_current(int item_ndx);
static void _item_select(int item_ndx);
static const menu_t* _pop_menu();
static bool _pop_to_menu(const menu_t* menu);
static void _show_current_menu();


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
    if (_active) {
        if (msg->t > (_hrc_last_ts + 200)) {
            _hrc_last_ts = msg->t;
            int16_t delta = msg->data.value16;
            //
            // If delta is negative, move menu item down, else up.
            if (delta > 0) {
                if (_item_current > 0) {
                    _item_make_current(_item_current - 1);
                }
            }
            else if (delta < 0) {
                if (_item_current < (_items_displayed - 1)) {
                    _item_make_current(_item_current + 1);
                }
            }
        }
    }
}

static void _handle_switch_action(cmt_msg_t* msg) {
    // Handle switch actions for the menuing
    //
    if (_active) {
        switch_id_t sw = msg->data.sw_action.switch_id;
        bool pressed = msg->data.sw_action.pressed;
        bool longpress = msg->data.sw_action.longpress;

        if (sw == SW_ATTNCMD) {
            // Command/Attention Switch
            if (longpress) {
                // Move to the Main Menu
                if (_main_menu) {
                    _pop_to_menu(_main_menu);
                    _show_current_menu();
                }
            }
            else if (!pressed) {
                // Move up one item on release if possible
                if (_menus_tail->prev) {
                    _pop_menu();
                    _show_current_menu();
                }
            }
        }
        else {
            // Rotary PB Switch
            if (!pressed) {
                // Select the item on release
                _item_select(_item_current);
            }
        }
        msg->abort = true; // We handled this, so don't have others get it.
    }
}


// ====================================================================
// Local/Private Methods
// ====================================================================

static void _item_display(int item_ndx, bool inverse) {
    const menu_item_t* item = _items_on_display[item_ndx];
    const char* label = item->get_label(_menus_tail->menu, item);
    int row = item_ndx + 1;
    uint8_t attr = (inverse ? DISP_CHAR_INVERT_BIT : 0);
    bool instr = true;
    for (int i = 0; i < _disp_info.cols; i++) {
        instr = instr && label[i];
        uint8_t c = (instr ? label[i] : SPCH) | attr;
        display_char(row, i, c, false, false);
    }
    display_paint();
}

static void _item_make_current(int item_ndx) {
    if (_item_current != NONE_NDX) {
        // Paint the current item normally
        _item_display(_item_current, false);
    }
    _item_current = item_ndx;
    _item_display(_item_current, true); // Print the item reversed
}

static void _item_select(int item_ndx) {
    const menu_item_t* item = _items_on_display[item_ndx];
    if (item->handler) {
        const menu_t* menu = _menus_root->menu;
        item->handler(menu, item);
    }
}

static int _items_display(const menu_t* menu, const menu_item_t* item1) {
    // Display items until we run out or hit the end of the display
    int row = 1;
    int items_displayed = 0;
    _items_on_display[0] = NULL;
    _items_on_display[1] = NULL;
    const menu_item_t* last = NULL;
    while (row < _disp_info.rows) {
        // Get an item
        const menu_item_t* item = menu->get_item(menu, last);
        _items_on_display[items_displayed] = item;
        if (!item) {
            break; // No more items
        }
        _item_display(items_displayed, false);
        items_displayed++;
        row++;
        last = item;
    }
    // If there are more items in the menu, put a DOWN-ARROW at the bottom right
    if (last && menu->has_item(menu, last)) {
        uint8_t da = 0x1A;
        display_char(_disp_info.rows - 1, _disp_info.cols - 1, da, false, Paint);
    }

    return (items_displayed);
}

static const menu_t* _pop_menu() {
    stack_menu_t* current = _menus_tail;
    // pop current off
    if (current) {
        _menus_tail = current->prev;
        free(current);
    }
    const menu_t* menu = (_menus_tail ? _menus_tail->menu : NULL);

    return (menu);
}

static bool _pop_to_menu(const menu_t* menu) {
    // See if the menu exists in the stack
    stack_menu_t* current = _menus_tail;
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
    const menu_t* cm;
    do {
        cm = _pop_menu();
    } while(cm != menu);
    return (true);
}

static void _push_menu(const menu_t* menu) {
    size_t ms = sizeof(stack_menu_t);
    stack_menu_t* si = (stack_menu_t *)malloc(ms);
    if (!si) {
        board_panic("!menumgr: malloc failed for _push_menu");
    }
    memset(si, 0, ms);
    si->menu = menu;
    if (_menus_root == NULL) {
        _menus_root = si;
        _menus_tail = si;
    }
    else {
        si->prev = _menus_tail;
        _menus_tail = si;
    }
}

static void _show_current_menu() {
    stack_menu_t* cmenu = _menus_tail; // Get the item at the tail of the stack
    const menu_t* menu = cmenu->menu; // Just to make things shorter
    display_clear(true);
    unsigned short int row = 0;
    unsigned short int col = 0;
    const char* title = menu->get_title(menu);
    int len = min(strlen(title), _disp_info.cols); // For centering
    int pad = constrain((_disp_info.cols - len), 0, _disp_info.cols);
    int p1 = pad/2;
    unsigned short int i;
    // Print the title centered and the whole row underlined.
    for (i = 0; i < p1; i++) {
        display_char(row, col++, SPCH, true, NoPaint); // underline
    }
    for (i = 0; i < len; i++) {
        display_char(row, col++, title[i], true, NoPaint);
    }
    while (col < _disp_info.cols) {
        display_char(row, col++, SPCH, true, NoPaint); // underline
    }
    display_paint();
    _items_displayed = _items_display(menu, NULL);
    if (_items_displayed > 0) {
        _item_make_current(0);
    }
}

// ====================================================================
// Public Methods
// ====================================================================

void menu_enter(const menu_t* menu) {
    if (!_pop_to_menu(menu)) {
        _push_menu(menu);
    }
    _show_current_menu();
    _active = true;
}

void menu_main_set(const menu_t* menu) {
    _main_menu = menu;
}


// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================


void menumgr_minit() {
    if (_initialized) {
        board_panic("!!! menumgr_minit: Called more than once !!!");
    }
    _menus_tail = _menus_root = NULL;
    _main_menu = NULL;

    // Get the info for the display to use while building screens.
    _disp_info = display_info();
    // Allocate memory for a list of displayed menu items.
    _items_on_display = malloc((_disp_info.rows+1) * sizeof(menu_item_t));
    _item_current = NONE_NDX;
    if (!_items_on_display) {
        board_panic("! menumgr: Could not malloc for list of displayed items !");
    }
    // Add our message handlers
    cmt_msg_hdlr_add(MSG_ROTARY_CHG, _handle_rotary_change);
    cmt_msg_hdlr_add(MSG_SW_ACTION, _handle_switch_action);
}
