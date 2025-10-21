/**
 * Interactive Shell - On the terminal.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef SHELL_H_
#define SHELL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "cmt/cmt.h"
#include "term/term.h"

#define shell_NAME_VERSION "AES v0.1"

// NOTE: Terminal line and column numbers are 1-based.

#define shell_COLUMNS 80 // VT/ANSI indicate valid values are 80|132, emulators take others
#define shell_LINES 36 // VT/ANSI indicate: 24, 25, 36, 42, 48, 52, and 72 are valid

// Code display color
#define shell_CODE_COLOR_FG TERM_CHR_COLOR_GREEN
#define shell_CODE_COLOR_BG TERM_CHR_COLOR_BLACK

// Command color
#define shell_CMD_COLOR_FG TERM_CHR_COLOR_BR_CYAN
#define shell_CMD_COLOR_BG TERM_CHR_COLOR_BLACK

// Top header and gap
#define shell_HEADER_COLOR_FG TERM_CHR_COLOR_BR_YELLOW
#define shell_HEADER_COLOR_BG TERM_CHR_COLOR_BLUE
#define shell_HEADER_INFO_LINE 1

// Current sender line (at the top)
#define shell_SENDER_COLOR_FG TERM_CHR_COLOR_BLUE
#define shell_SENDER_COLOR_BG TERM_CHR_COLOR_BR_YELLOW
#define shell_SENDER_LINE (2)

// Station list
#define shell_STATION_LIST_COLOR_FG TERM_CHR_COLOR_BR_MAGENTA
#define shell_STATION_LIST_BOX_COLOR_FG TERM_CHR_COLOR_MAGENTA
#define shell_STATION_LIST_COLOR_BG TERM_CHR_COLOR_BLACK
#define shell_STATIONS_PER_LINE (3)
#define shell_STATION_LIST_LAST_LINE (shell_LINES - 1)

// Bottom status
#define shell_STATUS_COLOR_FG TERM_CHR_COLOR_BR_YELLOW
#define shell_STATUS_COLOR_BG TERM_CHR_COLOR_BLUE
#define shell_STATUS_LINE (shell_LINES)
#define shell_STATUS_LOGO_COL (shell_COLUMNS - 2)
#define shell_STATUS_TIME_COL ((shell_COLUMNS / 2) - 3)

// Scroll margins
#define shell_SCROLL_START_LINE (3)

// Labels
#define AES_LOGO "ÆS"

typedef struct _TERM_COLOR_PAIR_ {
    term_color_t fg;
    term_color_t bg;
} term_color_pair_t;

#define shell_GETLINE_MAX_LEN_  256 // Maximum line length (including /0) for the `term_getline` function

/**
 * @brief Function prototype registered to handle control characters.
 * @ingroup ui
 *
 * These can be registered for one or more control characters. The control
 * characters are received during Command Shell idle state or during `term_ui_getline`.
 *
 * Note: During `term_ui_getline` the characters ^H (BS), ^J (NL), and ^M (CR)
 * are handled by the line construction and handler functions for these characters
 * will not be called. In addition, ^[ (ESC) is used to erase the line if a handler
 * is not registered.
 *
 * @param c The control character received.
 */
typedef void (*shell_control_char_handler)(char c);

/**
 * @brief Callback function that gets the line once it has been received/assembled.
 * @ingroup ui
 *
 * Because the system is message based for cooperative multi-tasking. It isn't possible
 * to block when trying to read a line from the terminal. To support getting an input
 * line from the user (echoing characters and handling backspace) as easily as possible
 * for things like the command processor and dialogs. The work is handled by the this
 * `term` module and a function is called from the main message loop once the line is
 * ready.
 *
 * @param line Pointer to the line buffer. The line buffer allows modification to support
 * parsing operations without needing to be copied. However, the buffer is statically
 * allocated and used for any call to `term_getline`. Therefore, if `term_getline` is
 * to be called while the current line contents are being used, the caller will need to
 * copy the line contents needed before making the call.
 *
 */
typedef void (*shell_getline_callback_fn)(char* line);

/**
 * @brief Function prototype for handling an terminal input character available notification.
 * @ingroup ui
 *
 * This is used to handle the input on the UI loop (core) rather than in an interrupt handler.
 */
typedef void (*shell_input_available_handler)(void);

/**
 * @brief Build (or rebuild) the UI on the terminal.
 * @ingroup ui
 */
extern void shell_build(void);

/**
 * @brief Get the current terminal color pair.
 * @ingroup ui
 */
extern term_color_pair_t shell_color_get();

/**
 * @brief Send the terminal the currently set text colors.
 * @ingroup ui
 */
extern void shell_color_refresh();

/**
 * @brief Set and save text forground and background colors.
 * @ingroup ui
 *
 * This should be used to set a screen color that can restored when needed.
 *
 * @param fg The color number for the foreground
 * @param bg The color number for the background
 */
extern void shell_color_set(term_color_t fg, term_color_t bg);

/**
 * @brief Get the control character handler for the given control character.
 * @ingroup ui
 *
 * @param c Character to get the handler for. Non control characters will return NULL.
 * @return shell_control_char_handler The handler for the character or NULL.
 */
static shell_control_char_handler shell_get_control_char_handler(char c);

/**
 * @brief Get a line of user input. Returns immediately. Calls back when line is ready.
 * @ingroup ui
 * @see term_getline_callback_fn for details on use.
 *
 * @param getline_cb The callback function to call when a line is ready.
 */
extern void shell_getline(shell_getline_callback_fn getline_cb);

/**
 * @brief Cancel a `shell_getline` that is inprogress.
 * @ingroup ui
 * @see shell_getline
 *
 * @param input_handler Input available handler to replace the `shell_getline` handler. Can be NULL.
 */
extern void shell_getline_cancel(shell_input_available_handler input_handler);

/**
 * @brief Handle a control character by calling the handler if one is registered.
 * @ingroup ui
 *
 * @param c The control character to handle.
 * @return true If there is a handler for this character
 * @return false If there isn't a handler for this character or the character isn't a control.
 */
extern bool shell_handle_control_character(char c);

/**
 * @brief Version of `printf` that adds a leading newline if interrupting
 *        printing of Morse Code.
 * @ingroup ui
 *
 * @return Number of characters printed.
 */
int shell_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));

/**
 * @brief Print the code-text string. This is 0-n spaces and a character.
 * @ingroup ui
 *
 * @param str The string to print.
 */
extern void shell_put_apptext(char* str);

/**
 * @brief Print a string in the scrolling (code) area of the screen.
 * @ingroup ui
 *
 * If code is displaying, this will print a newline and then the string.
 *
 * @param str The string to print.
 */
extern void shell_puts(char* str);

/**
 * @brief Register a control character handler.
 * @ingroup ui
 *
 * @see `shell_control_char_handler` for a description of when this is used.
 *
 * @param c The control character to register the handler for.
 * @param handler_fn The handler.
 */
extern void shell_register_control_char_handler(char c, shell_control_char_handler handler_fn);

/**
 * @brief Register a function to handle terminal input available.
 * @ingroup ui
 *
 * @param handler_fn
 */
extern void shell_register_input_available_handler(shell_input_available_handler handler_fn);

/**
 * @brief The line number of the end of the scroll region.
 *
 * @return uint16_t The line number, one-based.
 */
extern uint16_t shell_scroll_end_line_get();

/**
 * @brief Update the status bar.
 * @ingroup ui
 */
extern void shell_update_status();

/**
 * @brief Set the color to the output display color.
 * @ingroup ui
 */
extern void shell_use_output_color();

/**
 * @brief Set the color to the command display color.
 * @ingroup ui
 */
extern void shell_use_cmd_color();



extern void shell_module_init();

#ifdef __cplusplus
    }
#endif
#endif // SHELL_H_