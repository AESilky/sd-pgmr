/**
 * Copyright 2023 AESilky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "pico/stdlib.h"

typedef enum _display_attrs {
    DISP_ATTR_INVERSE   = 0x0001,
    DISP_ATTR_BLINK     = 0x0002,
    DISP_ATTR_BLANK     = 0x0004,
} display_attrs_t;

typedef struct _display_info {
    uint16_t hres;
    uint16_t vres;
    uint16_t cols;
    uint16_t rows;
    uint16_t colors;
    display_attrs_t attrs;
} display_info_t;

typedef struct _render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    int buflen;
} render_area_t;

#define Paint (true)
#define NoPaint (false)

/** @brief Memory area for the screen data pixel-bytes */
extern uint8_t display_buf[];
/** @brief Render area for the full display screen */
extern render_area_t display_full_area;
/** @brief Text character data for the full text screen */
extern char display_full_screen_text[];

/** @brief Bit to OR in to invert a character (display black char on white background) */
#define DISP_CHAR_INVERT_BIT 0x80
/** @brief Mask to AND with a character to remove invert (display white char on black background) */
#define DISP_CHAR_NORMAL_MASK 0x7F

extern void display_calc_render_area_buflen(render_area_t* area);

/** @brief Clear the text screen
 *  \ingroup display
 *
 * Clear the current text content and the screen.
 *
 *  \param paint Set true to paint the actual display. Otherwise, only buffers will be cleared.
*/
extern void display_clear(bool paint);

/** @brief Display a character on the text screen
 *  \ingroup display
 *
 * Display an ASCII character (plus some special characters).
 * If the top bit is set (c>127) the character is inverse (black on white background).
 *
 * @param row 0-5 With 0 being the top line
 * @param col 0-13 Starting column
 * @param c character to display
 * @param paint Set true to paint the actual display. Otherwise, only buffers will be updated.
 */
extern void display_char(unsigned short int row, unsigned short int col, const char c, bool paint);

extern void display_fill(uint8_t* buf, uint8_t fill_data);

extern void display_fill_page(uint8_t* buf, uint8_t fill_data, uint8_t page);

/** @brief Test the fonts by displaying all of the characters
 *  \ingroup display
 *
 * Display all of the font characters a page at a time. Pause between pages and overlap
 * the range of characters some from page to page.
 *
 */
extern void display_font_test(void);

extern display_info_t display_info();

/** @brief Paint the actual display screen
 *  \ingroup display
 *
 * To improve performance and the look of the display, most changes can be made without
 * updating the physical display. Then, once a batch of changes have been made, this
 * is called to move the screen/image buffer onto the display.
 */
extern void display_paint(void);

extern void display_render(uint8_t* buf, render_area_t* area);

/** @brief Clear the character row.
 *  \ingroup display
 *
 *  \param row The 0-based row to clear.
 *  \param paint True to also paint the screen.
*/
extern void display_row_clear(unsigned short int row, bool paint);

/** @brief Paint the portion of the screen containing the given character row.
 *  \ingroup display
 *
 *  This 'paints' the screen from the display buffer. To paint the buffer
 *  from the row data use `display_row_refresh`.
 *
 *  \param row The 0-based character row to paint.
*/
extern void display_row_paint(unsigned short int row);

/** @brief Scroll rows up.
 *  \ingroup display
 *
 *  Scroll the character rows up, removing the top row and
 *  clearing the bottom row.
 *
 *  \param row_t The 0-based top row.
 *  \param row_b The 0-based bottom row.
 *  \param paint True to paint the screen after the operation.
*/
extern void display_rows_scroll_up(unsigned short int row_t, unsigned short int row_b, bool paint);

/** @brief Display a string
 *  \ingroup display
 *
 * Display a string of ASCII characters (plus some special characters)
 *
 * @param row 0 being the top line
 * @param col 0 being the starting column
 * @param pString Pointer to the first character of a null-terminated string
 * @param invert True to invert the characters
 * @param paint True to paint the screen after the operation
 */
extern void display_string(unsigned short int row, unsigned short int col, const char* pString, bool invert, bool paint);

/** @brief Update the display (graphics) buffer from the row data. Optionally paint the screen
 *  \ingroup display
 *
 *  \param paint True to paint the screen after the operation.
*/
extern void display_update(bool paint);

extern int disp_write_buf(uint8_t* buf, size_t len);


// ////////////////////////////////////////////////////////////////////////////////
// ////      Module Init                                                       ////
// ////////////////////////////////////////////////////////////////////////////////
//
/** @brief Initialize the display
 *  \ingroup display
 *
 * This must be called before using the display.
 *
 */
extern void display_module_init(void);


#ifdef __cplusplus
}
#endif
#endif // DISPLAY_H_
