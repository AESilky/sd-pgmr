/**
 * User Interface - On the terminal.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "shell.h"

#include "board.h"
#include "cmd.h"
#include "cmt.h"
#include "term.h"
#include "include/util.h"

#include "cmd/cmd_t.h"
#include "dbus/cmd/cmds.h"
#include "debugging/cmd/cmds.h"
#include "deviceops/cmd/cmds.h"
#include "picohlp/cmd/cmds.h"

#include "hardware/rtc.h"
#include "pico/printf.h"

#include <ctype.h>
#include <string.h>

#define ESC_NOT_IN_PROGRESS (-1)
#define ESC_CHARS_MAX 20

static bool _initialized;
static bool _started;

static term_color_t _color_term_text_current_bg;
static term_color_t _color_term_text_current_fg;

static shell_control_char_handler _control_char_handler[32]; // Room for a handler for each control character

static shell_escape_seq_handler _escseq_handler[_SEH_NUM]; // Room for the ESC Sequences that we support

static char _getline_buf[shell_GETLINE_MAX_LEN_];
static int16_t _getline_index;

static int _esc_collecting; // If -1, not collecting. Else, index to store the next received character until done.
static char _esc_collected[ESC_CHARS_MAX + 1]; // Room to collect characters for an escape sequence.

static bool _wraptext_on;
static int _wraptext_column;
static char _wraptext_line[2 * shell_COLUMNS];

static uint16_t _scroll_end_line;

static shell_input_available_handler _input_available_handler;
static shell_getline_callback_fn _getline_callback; // Function pointer to be called when an input line is ready


static bool _process_char(char c, bool process_ctrl);

static void _do_backspace() {
    if (_getline_index > 0) {
        _getline_index--;
        term_cursor_left_1();
        term_erase_char(1);
    }
    _getline_buf[_getline_index] = '\0';
}

static bool _handle_es_backspace(sescseq_t escseq, const char* escstr) {
    // Left-Arrow (ESC[D) can be typed rather than the Backspace.
    _do_backspace();
    return (true);
}


/**
 * @brief Message handler for `MSG_INPUT_CHAR_READY`
 * @ingroup ui
 *
 * @param msg Nothing important in the message.
 */
static void _shell_handle_input_char_ready(cmt_msg_t* msg) {
    if (NULL != _input_available_handler) {
        _input_available_handler();
    }
}

/**
 * @brief A `term_notify_on_input_fn` handler for input ready.
 * @ingroup ui
 */
static void _input_ready_hook(void) {
    // Since this is called by an interrupt handler,
    // post a UI message so that the input is handled
    // by the UI message loop.
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_INPUT_CHAR_READY);
    postAPPMsg(&msg);
    // The hook is cleared on notify, so hook ourself back in.
    term_register_notify_on_input(_input_ready_hook);
}

static void _draw_station_list_box(uint16_t lines) {
    // uint16_t nsls = shell_STATION_LIST_LAST_LINE - (lines);
    // // Save current cursor and set color
    // term_cursor_save();
    // term_set_origin_mode(TERM_OM_UPPER_LEFT);
    // term_color_fg(shell_STATION_LIST_BOX_COLOR_FG);
    // term_color_bg(shell_STATION_LIST_COLOR_BG);
    // if (nsls > _station_list_separator_line && _station_list_separator_line > 0) {
    //     // Need to erase lines from old box
    //     term_cursor_moveto(_station_list_separator_line, 1);
    //     for (int i = 0; i <= (shell_STATION_LIST_LAST_LINE - _station_list_separator_line); i++) {
    //         term_erase_line();
    //         term_cursor_down_1();
    //     }
    // }
    // // Move to begining of separator line and set
    // term_cursor_moveto(nsls, 1);
    // // Draw the top
    // term_charset(VT_100_LINEDRAW);
    // for (int i = 0; i < shell_COLUMNS; i++) {
    //     if (i == (shell_COLUMNS / shell_STATIONS_PER_LINE) || i == ((((shell_COLUMNS / shell_STATIONS_PER_LINE) * 2) + 1))) {
    //         putchar(VT_LD_TCT);
    //     }
    //     else {
    //         putchar(VT_LD_HOR);
    //     }
    // }
    // // Draw the bars
    // for (int i = nsls + 1; i <= nsls + lines; i++) {
    //     term_cursor_moveto(i, ((shell_COLUMNS / shell_STATIONS_PER_LINE) + 1));
    //     putchar(VT_LD_VER);
    //     term_cursor_right(shell_COLUMNS / shell_STATIONS_PER_LINE);
    //     putchar(VT_LD_VER);
    // }
    // term_charset(VT_ASCII);
    // // If the start line is different, change the scroll area
    // bool scroll_area_smaller = (nsls < _station_list_separator_line);
    // if (nsls != _station_list_separator_line) {
    //     _station_list_separator_line = nsls;
    //     _scroll_end_line = _station_list_separator_line - 1;
    //     term_set_margin_top_bottom(shell_SCROLL_START_LINE, _scroll_end_line);
    //     if (scroll_area_smaller) {
    //         // Restore the previous cursor and if the line is beyond the scroll area, adjust it.
    //         term_set_origin_mode(TERM_OM_IN_MARGINS);
    //         term_cursor_restore();
    //         scr_position_t pos = term_get_cursor_position();
    //         if (pos.line > _scroll_end_line) {
    //             term_cursor_moveto(_scroll_end_line, pos.column);
    //         }
    //         return;
    //     }
    // }
    // // Put screen back
    // term_set_origin_mode(TERM_OM_IN_MARGINS);
    // term_cursor_restore();
}

static shell_control_char_handler _get_control_char_handler(char c) {
    if (iscntrl(c)) {
        return _control_char_handler[(int)c];
    }
    return (NULL);
}

static shell_escape_seq_handler _get_escseq_handler(sescseq_t escseq) {
    return _escseq_handler[escseq];
}

static void _getline_continue() {
    int ci;

    // Process characters that are available.
    while ((ci = term_getc()) >= 0) {
        char c = (char)ci;
        if (!_process_char(c, true)) {
            // See if there is a handler registered for this, else BEEP
            if (!shell_handle_control_character(c)) {
                // Control or 8-bit character we don't deal with
                putchar(BEL);
            }
        }
        // `while` will see if there are more chars available
    }
    // No more input chars are available, but we haven't gotten EOL yet,
    // hook for more to wake back up...
    term_register_notify_on_input(_input_ready_hook);
}

static void _header_fill_fixed() {
    term_cursor_save();
    term_set_origin_mode(TERM_OM_UPPER_LEFT);
    term_color_fg(shell_HEADER_COLOR_FG);
    term_color_bg(shell_HEADER_COLOR_BG);
    term_cursor_moveto(shell_HEADER_INFO_LINE, 1);
    term_erase_line();
    term_color_default();
    term_set_origin_mode(TERM_OM_IN_MARGINS);
    term_cursor_restore();
}

static bool _process_char(char c, bool process_ctrl) {
    bool processed = false;
    shell_getline_callback_fn fn = _getline_callback;

    if (process_ctrl) {
        // See if we are processing an ESC sequence.
        if (_esc_collecting >= 0) {
            if (_esc_collecting == 0) {
                // We are looking for CSI, see if this is it.
                if ('[' == c) {
                    _esc_collected[_esc_collecting++] = c;
                    _esc_collected[_esc_collecting] = '\0';
                    processed = true;
                }
                else {
                    _esc_collecting = ESC_NOT_IN_PROGRESS;
                }
            }
            else {
                // We are into the sequence, see if we have collected the whole thing.
                //
                _esc_collected[_esc_collecting++] = c;
                _esc_collected[_esc_collecting] = '\0';
                // 'Up Arrow' "CSI A"
                if (_esc_collecting == 2 && 'A' == c) {
                    // This is 'Up Arrow'
                    shell_escape_seq_handler fn = _get_escseq_handler(SES_KEY_ARROW_UP);
                    if (fn) {
                        processed = fn(SES_KEY_ARROW_UP, _esc_collected);
                    }
                }
                // 'Left Arrow' "CSI D"
                if (_esc_collecting == 2 && 'D' == c) {
                    // This is 'Left Arrow'
                    shell_escape_seq_handler fn = _get_escseq_handler(SES_KEY_ARROW_LF);
                    if (fn) {
                        processed = fn(SES_KEY_ARROW_LF, _esc_collected);
                    }
                }
                _esc_collecting = ESC_NOT_IN_PROGRESS;
            }
            if (_esc_collecting >= ESC_CHARS_MAX) {
                // We've collected as many characters as we can hold and we haven't received
                // something that we can process. Reset the ESC state.
                _esc_collecting = ESC_NOT_IN_PROGRESS;
            }
        }
        else {
            switch (c) {
            case '\n':
            case '\r':
                // EOL - Terminate the input line and give to callback.
                _getline_buf[_getline_index] = '\0';
                _getline_index = 0;
                _getline_callback = NULL;
                shell_register_input_available_handler(NULL);
                fn(_getline_buf);
                return (true);
                break;
            case BS:
            case DEL:
                // Backspace/Delete - move back if we aren't at the BOL
                _do_backspace();
                processed = true;
                break;
            case ESC:
                // First, see if there is a handler registered for ESC. If so, let it handle it.
                processed = shell_handle_control_character(c);
                if (!processed) {
                    // Escape sequence. Most begin with CSI (ESC[)
                    _esc_collecting = 0; // mark that we need the first char of the sequence
                    _esc_collected[0] = '\0';
                    processed = true;;
                }
                break;
            case '\030':
                // ^X Erases the current input
                while (_getline_index > 0) {
                    _getline_buf[_getline_index] = '\0';
                    term_cursor_left_1();
                    term_erase_char(1);
                    _getline_index--;
                }
                _getline_index = 0;
                _getline_buf[_getline_index] = '\0';
                processed = true;
                break;
            }
        }
    }
    if (!processed && c >= ' ' && c < DEL) {
        if (_getline_index < (shell_GETLINE_MAX_LEN_ - 1)) {
            _getline_buf[_getline_index++] = c;
            putchar(c);
        }
        else {
            // Alert them that they are at the end
            putchar(BEL);
        }
        processed = true;
    }
    return (processed);
}

static void _status_fill_fixed() {
    term_cursor_save();
    term_color_fg(shell_STATUS_COLOR_FG);
    term_color_bg(shell_STATUS_COLOR_BG);
    term_set_origin_mode(TERM_OM_UPPER_LEFT);
    term_cursor_moveto(shell_STATUS_LINE, 1);
    term_erase_line();
    printf("%s", shell_NAME_VERSION);
    term_cursor_moveto(shell_STATUS_LINE, shell_STATUS_LOGO_COL);
    printf("%s", AES_LOGO);
    term_set_origin_mode(TERM_OM_IN_MARGINS);
    term_cursor_restore();
}

static void _term_init() {
    _wraptext_on = false;
    memset(_wraptext_line, 0, sizeof(_wraptext_line));
    _wraptext_column = 0;
    _input_available_handler = NULL;
    // Clear out the control handlers and escape handlers
    memset(_control_char_handler, 0, sizeof(_control_char_handler));
    memset(_escseq_handler, 0, sizeof(_escseq_handler));
    // Register escape sequence handlers
    shell_register_esc_seq_handler(SES_KEY_ARROW_LF, _handle_es_backspace);
    //
    term_reset();
    term_color_default();
    term_set_type(VT_510_TYPE_SPEC, VT_510_ID_SPEC);
    term_set_title(shell_NAME_VERSION);
    term_set_size(shell_LINES, shell_COLUMNS);
    term_clear(true);
    //_draw_station_list_box(0);
    term_cursor_on(false);
    term_cursor_moveto(1,1);
    shell_use_output_color();
}

void shell_build(void) {
    term_color_default();
    term_text_normal();
}

term_color_pair_t shell_color_get() {
    term_color_pair_t tc = { _color_term_text_current_fg, _color_term_text_current_bg };
    return (tc);
}

void shell_color_refresh() {
    term_color_bg(_color_term_text_current_bg);
    term_color_fg(_color_term_text_current_fg);
}

void shell_color_set(term_color_t fg, term_color_t bg) {
    _color_term_text_current_bg = bg;
    _color_term_text_current_fg = fg;
    term_color_bg(bg);
    term_color_fg(fg);
}

void shell_getline(shell_getline_callback_fn getline_cb) {
    _getline_callback = getline_cb;
    shell_register_input_available_handler(_getline_continue);
    // Use the 'continue' function to process
    _getline_continue();
}

void shell_getline_append(const char* appndstr) {
    char c;
    while ((c = *appndstr++) && (_getline_index < (shell_GETLINE_MAX_LEN_ - 1))) {
        _process_char(c, false);
    }
}

void shell_getline_cancel(shell_input_available_handler input_handler) {
    _getline_callback = NULL;
    shell_register_input_available_handler(input_handler);
    _getline_index = 0;
    _getline_buf[_getline_index] = '\0';
}

bool shell_handle_control_character(char c) {
    if (iscntrl(c)) {
        shell_control_char_handler handler_fn = _get_control_char_handler(c);
        if (handler_fn) {
            handler_fn(c);
            return (true);
        }
    }
    return (false);
}

static void _printc_for_printf_term(char c, void* arg) {
    putchar(c);
}

int shell_printf(const char* format, ...) {
    int pl = 0;
    // if (_wraptext_on) {
    //     putchar('\n');
    //     pl = 1;
    // }
    va_list xArgs;
    va_start(xArgs, format);
    pl += vfctprintf(_printc_for_printf_term, NULL, format, xArgs);
    va_end(xArgs);

    return (pl);
}

int shell_printferr(const char* format, ...) {
    term_color_pair_t cs = shell_color_get();
    shell_color_set(TERM_CHR_COLOR_BR_RED, TERM_CHR_COLOR_BLACK);
    int pl = 0;
    // if (_wraptext_on) {
    //     putchar('\n');
    //     pl = 1;
    // }
    va_list xArgs;
    va_start(xArgs, format);
    pl += vfctprintf(_printc_for_printf_term, NULL, format, xArgs);
    va_end(xArgs);
    shell_color_set(cs.fg, cs.bg);

    return (pl);
}

static void _putchar_for_app(char c) {
    if ('\n' == c) {
        putchar(c);
        _wraptext_column = 0;
        return;
    }
    if (_wraptext_column == shell_COLUMNS) {
        // Printing this will cause a wrap.
        if (' ' == c) {
            // It's a space. Just print a newline instead.
            putchar('\n');
            _wraptext_column = 0;
            return;
        }
        else {
            // See if we can move back to a space
            int i = 0;
            for (; i < _wraptext_column; i++) {
                if (' ' == _wraptext_line[_wraptext_column - i]) {
                    break;
                }
            }
            if (i < _wraptext_column) {
                // Yes there was a space in the line. Backup, print a '\n', then reprint to the end of line.
                term_cursor_left(i-1);
                term_erase_eol();
                putchar('\n');
                int nc = 0;
                for (int j = ((_wraptext_column - i) + 1); j < _wraptext_column; j++) {
                    putchar(_wraptext_line[j]);
                    nc++;
                }
                _wraptext_column = nc;
            }
            else {
                // No spaces in the current line. Just print a '\n' (breaking the word).
                putchar('\n');
                _wraptext_column = 0;
            }
        }
    }
    _wraptext_line[_wraptext_column] = c;
    putchar(c);
    _wraptext_column++;
    if ('=' == c) {
        putchar('\n');
        _wraptext_column = 0;
    }
}

void shell_put_apptext(char* str) {
    // If the Command Shell is active, don't display output.
    if (CMD_SNOOZING == cmd_get_state()) {
        if (!_wraptext_on) {
            _putchar_for_app('\n');
            _wraptext_on = true;
        }
        char c;
        while ('\0' != (c = *str++)) {
            _putchar_for_app(c);
        }
    }
}

void shell_putc(uint8_t c) {
    putchar(c);
}

void shell_puts(char* str) {
    if (_wraptext_on) {
        putchar('\n');
        _wraptext_on = false;
    }
    printf("%s", str);
}

void shell_register_control_char_handler(char c, shell_control_char_handler handler_fn) {
    if (iscntrl(c)) {
        _control_char_handler[(int)c] = handler_fn;
    }
}

void shell_register_esc_seq_handler(sescseq_t escseq, shell_escape_seq_handler handler_fn) {
    _escseq_handler[escseq] = handler_fn;
}

void shell_register_input_available_handler(shell_input_available_handler handler_fn) {
    _input_available_handler = handler_fn;
}

uint16_t shell_scroll_end_line_get() {
    return _scroll_end_line;
}

void shell_update_status() {
    // Put the current time in the center
    char buf[10];
    datetime_t now;

    rtc_get_datetime(&now);
    strdatetime(buf, 9, &now, SDTC_TIME_2CHAR_HOUR | SDTC_TIME_AMPM);
    term_color_pair_t tc = shell_color_get();
    term_cursor_save();
    term_color_fg(shell_STATUS_COLOR_FG);
    term_color_bg(shell_STATUS_COLOR_BG);
    term_set_origin_mode(TERM_OM_UPPER_LEFT);
    term_cursor_moveto(shell_STATUS_LINE, shell_STATUS_TIME_COL);
    printf("%s", buf);
    term_set_origin_mode(TERM_OM_IN_MARGINS);
    term_cursor_restore();
    shell_color_set(tc.fg, tc.bg);
}

void shell_use_output_color() {
    shell_color_set(shell_CODE_COLOR_FG, shell_CODE_COLOR_BG);
}

void shell_use_cmd_color() {
    shell_color_set(shell_CMD_COLOR_FG, shell_CMD_COLOR_BG);
}



void shell_start() {
    if (_started) {
        board_panic("!!! Shell should only be started once. !!!");
    }
    _started = true;
    shell_build();

    _term_init();
    term_text_normal();
    cmd_minit();

    // Initialize all of the modules that have commands
    //
    dbcmds_minit();
    dbuscmds_minit();   // Data Bus shell commands
    pdcmds_minit();     // Programmable Device (Flash) shell commands
    picocmds_minit();   // Pico Util/Control shell commands

    // Activate the command processor
    cmd_activate(true);
}

void shell_minit() {
    if (_initialized) {
        board_panic("!!! shell_modinit already called. !!!");
    }
    _initialized = true;

    term_minit();
    _esc_collecting = ESC_NOT_IN_PROGRESS;
    //
    // Register our message handler
    cmt_msg_hdlr_add(MSG_INPUT_CHAR_READY, _shell_handle_input_char_ready);
}