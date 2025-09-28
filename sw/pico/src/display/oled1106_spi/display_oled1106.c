/**
 * Copyright 2023 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "system_defs.h"
#include "board.h"

#include "display/fonts/font_9_10_h.h"
#include "display_oled1106.h"
#include "oled1106_spi.h"

#include <string.h>
#include "pico/stdio.h"

/*! @brief Memory display_full_area for a screen of text (characters) */
char o1106_full_screen_text[DISP_CHAR_LINES * DISP_CHAR_COLS];

/*
 * This must be called before using the display.
 */
void display_module_init(void) {
    // run through the complete initialization process
    oled_module_init();
    display_clear(true);
}

/*
 * Clear the current text content and the screen.
 *
 *  \param paint Set true to paint the actual display. Otherwise, only buffers will be cleared.
*/
void display_clear(bool paint) {
    memset(o1106_full_screen_text, 0x00, sizeof(o1106_full_screen_text));
    display_fill(display_buf, 0x00);
    if (paint) {
        display_paint();
    }
}

/*
 * Display an ASCII character (plus some special characters)
 * If the top bit is set (c>127) the character is inverse (black on white background).
 *
 * The font characters are 9x10 allowing for 6 lines of 14 characters
 * on the 128x64 dot display.
 *
 * @param row 0-5
 * @param col 0-13
 * @param c character to display
 *
 * Using a 10 pixel high font on an 64 pixel high screen with 8 pixel high pages,
 * gives us a memory row layout as follows:
 *
 * PxR | Page | Row
 * -----------------
 *   0 |    0 |  0
 *   7 |      |
 *  ----------|
 *   8 |    1 |
 *    -|      |-----
 *  10 |      |  1
 *  15 |      |
 *  ----------|
 *  16 |    2 |
 *    -|      |-----
 *  20 |      |  2
 *  .. |      |
 *  23 |      |
 *  ----------|
 *  24 |    3 |
 *  .. |      |
 *  29 |      |
 *    -|      |-----
 *  30 |      |  3
 *  31 |      |
 *  ----------|
 *  32 |    4 |
 *  .. |      |
 *  39 |      |
 *  ----------|-----
 *  40 |    5 |  4
 *  .. |      |
 *  47 |      |
 *  ----------|
 *  48 |    6 |
 *  49 |      |
 *    -|      |-----
 *  50 |      |  5
 *  55 |      |
 *  ----------|
 *  56 |    7 |
 *  .. |      |
 *  59 |      |
 *    -|      |-----
 *  60 |      | BLANK
 *  .. |      |   |
 *  63 |      | BLANK
 *  ----------------
 *
 * Therefore; writing a character into row-col on the screen requires that it be merged (or'ed) in with
 * existing buffer data from two pages using masks and shifts that depend on what row is being written.
 * The masks are:
 * Row 1 l: P0:00 <0
 *       h: P1:FE
 *
 * Row 2 l: P1:01 <2
 *       h: P2:FC
 *
 * Row 3 l: P2:03 <4
 *       h: P3:F8
 *
 * Row 4 l: P3:07 <6
 *       h: P4:F0
 *
 * Row 5 l: P4:0F <0
 *       h: P5:E0
 *
 * Row 6 l: P5:1F <2
 *       h: P6:C0
 *
 * Row 7 l: P6:3F <6
 *          P7:00 (special mask, since the bottom 2 rows are blank)
 *
 * An additional complication comes from the font being 9 bits wide and the display
 * being 128 bits wide. This allows 14 chars with 2 dot columns left over.
 * These extra pixels must be accounted for when indexing into the display buffer.
 *
 * An additional, additional, complication comes from the SH1106 controller having
 * 132 bits wide, even though the LCD panel is only 128. This means that we need to
 * start each bit-row at dot column 2 rather than 0.
 */
void display_char(unsigned short int row, unsigned short int col, const char c, bool paint) {
    if (row >= DISP_CHAR_LINES || col >= DISP_CHAR_COLS) {
        return;  // Invalid row or column
    }
    *(o1106_full_screen_text + (row * DISP_CHAR_COLS) + col) = c;
    unsigned char cl = c & 0x7F;
    // Calculate the display pages the character falls into,
    // the font data shift, and the mask required.
    uint8_t pagel = (row * FONT_HEIGHT) / (OLED_PAGE_HEIGHT);
    uint16_t offsetl = pagel * OLED_WIDTH;
    uint16_t offseth = (pagel + 1) * OLED_WIDTH;
    uint8_t shift = ((FONT_HEIGHT - OLED_PAGE_HEIGHT) * row) % OLED_PAGE_HEIGHT;
    uint16_t mask = (FONT_BIT_MASK << shift) ^ 0xFFFF;
    uint16_t invert_mask = 0x0000;
    if (c & DISP_CHAR_INVERT_BIT) {
        invert_mask = 0x03FF << shift;
    }
    for (int i = 0; i < FONT_WIDTH; i++) {
        uint16_t cdata = Font_Table[(cl * FONT_WIDTH) + i] << shift;
        cdata = cdata ^ invert_mask;
        // Read the existing data
        uint16_t indx_l = offsetl + ((col * FONT_WIDTH) + i) + OLED_DEAD_LEFT;
        uint16_t indx_h = offseth + ((col * FONT_WIDTH) + i) + OLED_DEAD_LEFT;
        uint8_t edata_l = display_buf[indx_l];
        uint8_t edata_h = display_buf[indx_h];
        uint16_t edata = edata_h << 8 | edata_l;
        // create the result
        uint16_t rdata = (edata & mask) | cdata;
        // Write the data back to the buffer
        display_buf[indx_l] = LOWBYTE(rdata);
        display_buf[indx_h] = HIGHBYTE(rdata);
    }
    if (paint) {
        display_paint();
    }
}

/** @brief Paint the physical screen
 */
void display_paint(void) {
    display_render(display_buf, &display_full_area);
}

/** @brief Clear the character row.
 *  \ingroup display
 *
 *  \param row The 0-based row to clear.
 *  \param paint True to paint the screen.
*/
void display_row_clear(unsigned short int row, bool paint) {
    if (row >= DISP_CHAR_LINES) {
        return;  // Invalid row
    }
    memset((o1106_full_screen_text + (row * DISP_CHAR_COLS)), 0x00, DISP_CHAR_COLS);
    // Calculate the display pages the character falls into
    // and the mask required.
    uint8_t pagel = (row * FONT_HEIGHT) / (OLED_PAGE_HEIGHT);
    uint16_t offsetl = pagel * OLED_WIDTH;
    uint16_t offseth = (pagel + 1) * OLED_WIDTH;
    uint8_t shift = ((FONT_HEIGHT - OLED_PAGE_HEIGHT) * row) % OLED_PAGE_HEIGHT;
    uint16_t mask = (FONT_BIT_MASK << shift) ^ 0xFFFF;
    for (int i = 0; i < OLED_WIDTH; i++) {
        // Read the existing data
        uint16_t indx_l = offsetl + i;
        uint16_t indx_h = offseth + i;
        uint8_t edata_l = display_buf[indx_l];
        uint8_t edata_h = display_buf[indx_h];
        uint16_t edata = edata_h << 8 | edata_l;
        // create the result
        uint16_t rdata = (edata & mask);
        // Write the data back to the buffer
        display_buf[indx_l] = LOWBYTE(rdata);
        display_buf[indx_h] = HIGHBYTE(rdata);
    }
    if (paint) {
        display_paint();
    }
}

/**
 * Update the portion of the screen containing the given character row.
 */
void display_row_paint(unsigned short int row) {
    if (row >= DISP_CHAR_LINES) {
        return;  // Invalid row
    }
    // Calculate the display page the row falls into,
    uint8_t pagel = (row * FONT_HEIGHT) / (OLED_PAGE_HEIGHT);
    render_area_t row_area = {start_col: 0, end_col : OLED_WIDTH - 1, start_page : pagel, end_page : pagel + 1};
    calc_render_area_buflen(&row_area);
    display_render(display_buf + (pagel * OLED_WIDTH), &row_area);
}

/** Scroll 2 or more rows up.
 *
 *  Scroll the character rows up, removing the top row and
 *  clearing the bottom row.
 *
 *  \param row_t The 0-based top row.
 *  \param row_b The 0-based bottom row.
 *  \param paint True to paint the screen after the operation.
*/
void display_rows_scroll_up(unsigned short int row_t, unsigned short int row_b, bool paint) {
    if (row_b <= row_t || row_b >= DISP_CHAR_LINES) {
        return;  // Invalid row value
    }
    memmove((o1106_full_screen_text + (row_t * DISP_CHAR_COLS)), (o1106_full_screen_text + ((row_t + 1) * DISP_CHAR_COLS)), (row_b - row_t) * DISP_CHAR_COLS);
    memset((o1106_full_screen_text + (row_b * DISP_CHAR_COLS)), 0x00, DISP_CHAR_COLS);
    display_update(paint);
}

/** @brief Display a string
 *  \ingroup display
 *
 * Display a string of ASCII characters (plus some special characters)
 *
 * @param row 0-6 With 0 being the top line
 * @param col 0-11 Starting column
 * @param pString Pointer to the first character of a null-terminated string
 * @param invert True to invert the characters
 * @param paint True to paint the screen after the operation
 */
void display_string(unsigned short int row, unsigned short int col, const char *pString, bool invert, bool paint) {
    if (row >= DISP_CHAR_LINES || col >= DISP_CHAR_COLS) {
        return;  // Invalid row or column
    }
    for (unsigned char c = *pString; c != 0; c = *(++pString)) {
        if (invert) {
            c = c ^ DISP_CHAR_INVERT_BIT;
        }
        display_char(row, col, c, false);
        col++;
        if (col == DISP_CHAR_COLS) {
            col = 0;
            row++;
            if (row == DISP_CHAR_LINES) {
                break;
            }
        }
    }
    if (paint) {
        display_paint();
    }
}

/** @brief Update the display buffer from the character row data
 *
 *  \param paint True to paint the display after the operation.
*/
void display_update(bool paint) {
    // TODO: More efficient way to do this...
    for (unsigned int r = 0; r < DISP_CHAR_LINES; r++) {
        for (unsigned int c = 0; c < DISP_CHAR_COLS; c++) {
            unsigned char d = *(o1106_full_screen_text + (r * DISP_CHAR_COLS) + c);
            display_char(r, c, d, false);
        }
        if (paint) {
            display_paint();
        }
    }
}

/*
 * Display all of the font characters a page at a time. Pause between pages and overlap
 * the range of characters some from page to page.
 */
void display_font_test(void) {
    // init display_render buffer

    // test font display 1
    uint8_t *ptr = display_buf;
    int start_char = 0;
    uint16_t mask = 0x00FF;
    uint8_t shift = 0;
    for (int j = 0; j < 8; j++) {
        if (j % 2 == 0) {
            mask = 0x00FF;
            shift = 0;
        }
        else {
            mask = 0xFF00;
            shift = 8;
        }
        // The display is 128 columns, but 14 characters is 126.
        // So we have 2 blank columns.
        *(ptr++) = 0x00;  // make first col blank
        for (int i = start_char + 0; i < (start_char + (14 * 9)); i++) {
            int ci = i + ((j/2)*(14*9));
            uint16_t d = Font_Table[ci];
            uint8_t bl = (uint8_t)((d & mask) >> shift);
            *(ptr++) = bl;
        }
        *(ptr++) = 0x00;  // make last col blank
    }
    display_render(display_buf, &display_full_area);

    sleep_ms(1000);

    // test font display 2
    ptr = display_buf;
    start_char = 0x20 * 9;
    mask = 0x00FF;
    shift = 0;
    for (int j = 0; j < 8; j++) {
        if (j % 2 == 0) {
            mask = 0x00FF;
            shift = 0;
        }
        else {
            mask = 0xFF00;
            shift = 8;
        }
        // The display is 128 columns, but 14 characters is 126.
        // So we have 2 blank columns.
        *(ptr++) = 0x00;  // make first col blank
        for (int i = start_char + 0; i < (start_char + (14 * 9)); i++) {
            int ci = i + ((j/2)*(14*9));
            uint16_t d = Font_Table[ci];
            uint8_t bl = (uint8_t)((d & mask) >> shift);
            *(ptr++) = bl;
        }
        *(ptr++) = 0x00;  // make last col blank
    }
    display_render(display_buf, &display_full_area);

    sleep_ms(1000);

    // test font display 3
    ptr = display_buf;
    start_char = 0x40 * 9;
    mask = 0x00FF;
    shift = 0;
    for (int j = 0; j < 8; j++) {
        if (j % 2 == 0) {
            mask = 0x00FF;
            shift = 0;
        }
        else {
            mask = 0xFF00;
            shift = 8;
        }
        // The display is 128 columns, but 14 characters is 126.
        // So we have 2 blank columns.
        *(ptr++) = 0x00;  // make first col blank
        for (int i = start_char + 0; i < (start_char + (14 * 9)); i++) {
            int ci = i + ((j/2)*(14*9));
            uint16_t d = Font_Table[ci];
            uint8_t bl = (uint8_t)((d & mask) >> shift);
            *(ptr++) = bl;
        }
        *(ptr++) = 0x00;  // make last col blank
    }
    display_render(display_buf, &display_full_area);

    sleep_ms(1000);

    // test font display 4
    ptr = display_buf;
    start_char = 0x60 * 9;
    mask = 0x00FF;
    shift = 0;
    for (int j = 0; j < 8; j++) {
        if (j % 2 == 0) {
            mask = 0x00FF;
            shift = 0;
        }
        else {
            mask = 0xFF00;
            shift = 8;
        }
        // The display is 128 columns, but 14 characters is 126.
        // So we have 2 blank columns.
        *(ptr++) = 0x00;  // make first col blank
        for (int i = start_char + 0; i < (start_char + (14 * 9)); i++) {
            int ci = i + ((j/2)*(14*9));
            if (ci > 127 * 9) {
                break;
            }
            uint16_t d = Font_Table[ci];
            uint8_t bl = (uint8_t)((d & mask) >> shift);
            *(ptr++) = bl;
        }
        *(ptr++) = 0x00;  // make last col blank
    }
    display_render(display_buf, &display_full_area);

    sleep_ms(1000);
}
