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
#include "dskops/cmd/cmds.h"
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

static bool _host_connected;
static bool _host_welcomed;

static bool _wraptext_on;
static int _wraptext_column;
static char _wraptext_line[2 * shell_COLUMNS];

static uint16_t _scroll_end_line;

static shell_input_available_handler _input_available_handler;
static shell_getline_callback_fn _getline_callback; // Function pointer to be called when an input line is ready

static void _host_welcome();
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
 * @brief Handle Housekeeping tasks. This is triggered every ~16ms.
 *
 * For reference, 625 times is 10 seconds.
 *
 * @param msg Nothing important in the message.
 */
static void _handle_housekeeping(cmt_msg_t* msg) {
    static uint cnt = 0;

    if (cnt++ % 11 == 0) {
        if (stdio_usb_connected()) {
            if (!_host_connected) {
                _host_connected = true;
            }
            if (!_host_welcomed) {
                if (term_input_available()) {
                    _host_welcome();
                }
            }
        }
        else {
            _host_connected = false;
            _host_welcomed = false;
        }
    }
}

/**
 * @brief Message handler for `MSG_INPUT_CHAR_READY`
 * @ingroup app
 *
 * @param msg Nothing important in the message.
 */
static void _handle_input_char_ready(cmt_msg_t* msg) {
    if (NULL != _input_available_handler) {
        // If there is an input character, a host must be connected
        _host_connected = true;
        _input_available_handler();
    }
}

/**
 * @brief Init/ReInit the terminal (connected)
 * @ingroup app
 *
 * @param msg
 */
static void _handle_term_init(cmt_msg_t* msg) {
    _host_welcome();
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
        if (!_host_welcomed) {
            _host_welcome();
            continue;
        }
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

static void _host_welcome() {
    // Now to a full init of the terminal
    term_init();
    term_set_title(shell_NAME_VERSION);
    term_text_normal();
    // Tell the Host hello
    shell_puts("SilkyDESIGN Flash Programmer\n");
    _host_welcomed = true;
    _started = true;
    shell_build();
    cmd_activate(true);
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
                _input_available_handler = NULL; // Cleared when called
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
    if (_host_connected) {
        // if (_wraptext_on) {
        //     putchar('\n');
        //     pl = 1;
        // }
        va_list xArgs;
        va_start(xArgs, format);
        pl += vfctprintf(_printc_for_printf_term, NULL, format, xArgs);
        va_end(xArgs);
    }
    return (pl);
}

int shell_printferr(const char* format, ...) {
    int pl = 0;
    if (_host_connected) {
        term_color_pair_t cs = shell_color_get();
        shell_color_set(TERM_CHR_COLOR_BR_RED, TERM_CHR_COLOR_BLACK);
        // if (_wraptext_on) {
        //     putchar('\n');
        //     pl = 1;
        // }
        va_list xArgs;
        va_start(xArgs, format);
        pl += vfctprintf(_printc_for_printf_term, NULL, format, xArgs);
        va_end(xArgs);
        shell_color_set(cs.fg, cs.bg);
    }
    return (pl);
}

static void _putchar_for_app(char c) {
    if (_host_connected) {
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
    if (_host_connected) {
        putchar(c);
    }
}

void shell_puts(const char* str) {
    if (_host_connected) {
        if (_wraptext_on) {
            putchar('\n');
            _wraptext_on = false;
        }
        int len = (int)strlen(str);
        stdio_put_string(str, len, false, true);
        stdio_flush();
    }
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
    // Register our input handler with term
    term_register_notify_on_input(_input_ready_hook);
    // Do first init of the terminal. Will do another when we receive the first character.
    term_init1();
    term_text_normal();
    // Initialize the CMD module
    cmd_modinit();

    // Initialize all of the modules that have commands
    //
    dbcmds_modinit();
    dbuscmds_modinit();   // Data Bus shell commands
    diskcmds_modinit();   // Disk (SD Card) commands
    pdcmds_modinit();     // Programmable Device (Flash) shell commands
    picocmds_modinit();   // Pico Util/Control shell commands

    // Activate the command processor
    cmd_activate(true);
}

void shell_modinit() {
    if (_initialized) {
        board_panic("!!! shell_modinit already called. !!!");
    }
    _initialized = true;

    _esc_collecting = ESC_NOT_IN_PROGRESS;
    // Base terminal initialization
    term_init0();
    //
    // Register our message handlers
    cmt_msg_hdlr_add(MSG_INPUT_CHAR_READY, _handle_input_char_ready);
    cmt_msg_hdlr_add(MSG_CMD_INIT_TERMINAL, _handle_term_init);
    cmt_msg_hdlr_add(MSG_PERIODIC_RT, _handle_housekeeping);

}