/**
 * @brief OLED Display Panel Module for 1106 Display. I2C or SPI is selected by build/link.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "../display.h"
#include "../fonts/font_9_10_h.h"
#include "../dispops/dispops.h"
#include "oled1106.h"

#include <string.h>

// commands (see datasheet)
#define OLED_NOOP 0xE3              // Can be sent if needed
#define OLED_CONTRAST 0x81          // Next byte is contrast
#define OLED_ENTIRE_ONx 0xA4        // A4=Normal A5=Force ON
#define OLED_NORM_INVx 0xA6         // A6=Normal A7=Reverse

#define OLED_DISP_OFF_ONx 0xAE      // AE=OFF AF=ON
#define OLED_COL_ADDR_LOWx 0x00     // (0000 xxxx) Low column address
#define OLED_COL_ADDR_HIGHx 0x10    // (0001 xxxx) High column address
#define OLED_PAGE_ADDRx 0xB0        // (1011 xxxx) Page address
#define OLED_DISP_START_LINEx 0x40  // (01 xxxxxx) Ram display line for COM0
#define OLED_SEG_COL_MAPx 0xA0      // A0=Normal A1=Reverse
#define OLED_SEG_COL_MAP_NORM 0x00  //  The value to OR with the base command
#define OLED_SEG_COL_MAP_REV  0x01  //  The value to OR with the base command
#define OLED_MUX_RATIO 0xA8         // Next byte (0-63) is ratio
#define OLED_COM_ROW_DIRx 0xC0      // (1100 D000) Scan D=0 0-n, D=1 n-0 (C0|C8)
#define OLED_COM_ROW_DIR_DNORM 0x00 //  The 'D' to OR with the base command
#define OLED_COM_ROW_DIR_DREV  0x08 //  The 'D' to OR with the base command
#define OLED_DISP_OFFSET 0xD3       // Next byte (0-63) is offset
#define OLED_COM_PIN_CFG 0xDA       // Next byte controls common pads
#define OLED_DISP_CLK_DIV 0xD5      // Next byte controls clock divider (see datasheet)
#define OLED_PRECHARGE 0xD9         // Next byte controls pre-charge (Dis/Pre)
#define OLED_VCOM_DESEL 0xDB        // Next byte controls VCOM deselect level
#define OLED_CHARGE_PUMPx 0x32      // (0011 00xx) DC-DC output voltage
//
#define OLED_VRES 64
#define OLED_HRES 132
#define OLED_DEAD_LEFT 2
#define OLED_PAGE_HEIGHT 8
#define OLED_NUM_PAGES OLED_VRES / OLED_PAGE_HEIGHT
#define OLED_BUF_LEN (OLED_NUM_PAGES * OLED_HRES)
// ZZZ - Calculate these based on the font metrics
#define DISP_CHAR_LINES 6
#define DISP_CHAR_COLS 14
//

/** @brief Render area for the full display screen */
render_area_t display_full_area = { start_col: 0, end_col : OLED_HRES - 1, start_page : 0, end_page : OLED_NUM_PAGES - 1 };
/** @brief Text character data for the full text screen */
char display_full_screen_text[DISP_CHAR_LINES * DISP_CHAR_COLS];
/*! @brief Memory area for the screen data pixel-bytes */
uint8_t display_buf[OLED_BUF_LEN];

display_info_t _dinfo;

// ///////////////////////////////////////////////////////////////////////////////////
// ////  Private/Local Methods                                                    ////
// ///////////////////////////////////////////////////////////////////////////////////
//

static void _calc_render_area_buflen(render_area_t* area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

static void _display_clear() {
    uint16_t p, w;
    for (p = 0; p < OLED_NUM_PAGES; p++) {
        /* set page address */
        oled1106_send_cmd(OLED_PAGE_ADDRx | p);
        oled1106_send_cmd(OLED_COL_ADDR_LOWx | 0x00);
        oled1106_send_cmd(OLED_COL_ADDR_HIGHx | 0x00);
        disp_data_op_start();
        for (w = 0; w < OLED_HRES; w++) {
            disp_write(0x00);
        }
        disp_op_end();
    }
}

static void _oled1106_module_init(bool invert) {
    // Some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are all included
    // rather than rely on POR.
    uint8_t col_dir_value = (invert ? OLED_SEG_COL_MAP_REV : OLED_SEG_COL_MAP_NORM);
    uint8_t row_dir_value = (invert ? OLED_COM_ROW_DIR_DREV : OLED_COM_ROW_DIR_DNORM);

    // some configuration values are recommended by the board manufacturer

    // We must reset the display for it to function
    disp_reset();

    oled1106_send_cmd(OLED_DISP_OFF_ONx | 0x00);     // set display off

    oled1106_send_cmd(OLED_COL_ADDR_LOWx | 0x00);    // set the column address
    oled1106_send_cmd(OLED_COL_ADDR_HIGHx | 0x00);

    oled1106_send_cmd(OLED_DISP_START_LINEx | 0x00); // start at 0

    oled1106_send_cmd(OLED_CONTRAST);
    oled1106_send_cmd(0x80);                        // Mid contrast to start

    oled1106_send_cmd(OLED_SEG_COL_MAPx | col_dir_value);    // Normal segment/column mapping (0=Normal 1=Reverse)
    oled1106_send_cmd(OLED_COM_ROW_DIRx | row_dir_value);    // Vertical Scan direction
    oled1106_send_cmd(OLED_NORM_INVx | 0x00);       // Set normal

    oled1106_send_cmd(OLED_MUX_RATIO);
    oled1106_send_cmd(0x3F);                        // 1/64 ratio

    oled1106_send_cmd(OLED_DISP_OFFSET);
    oled1106_send_cmd(0x00);                        // No offset

    oled1106_send_cmd(OLED_DISP_CLK_DIV);
    oled1106_send_cmd(0x80);                        // (xxxx yyyy) x=+15% y=f/1 (100fps)

    oled1106_send_cmd(OLED_PRECHARGE);
    oled1106_send_cmd(0xF1);                        // Dis-Charge 15 Clocks & Pre-Charge 1 Clock

    oled1106_send_cmd(OLED_COM_PIN_CFG);
    oled1106_send_cmd(0x12);                        // (000D 0010) D=1 Alternative

    oled1106_send_cmd(OLED_VCOM_DESEL);
    oled1106_send_cmd(0x40);                        // Beta=1

    oled1106_send_cmd(OLED_PAGE_ADDRx | 0x00);      // Page 0
    oled1106_send_cmd(OLED_NORM_INVx | 0x00);       // Normal display

    // initialize render area for entire display (128 pixels by 8 pages)
    _calc_render_area_buflen(&display_full_area);

    // zero the entire display
    _display_clear();
    display_fill(display_buf, 0x00);
    display_render(display_buf, &display_full_area);

    oled1106_send_cmd(OLED_DISP_OFF_ONx | 0x01);    // Turn display on

    // intro sequence: flash the screen twice
    for (int i = 0; i < 2; i++) {
        oled1106_send_cmd(OLED_ENTIRE_ONx | 0x01); // ignore RAM, all pixels on
        sleep_ms(250);
        oled1106_send_cmd(OLED_ENTIRE_ONx | 0x00); // go back to following RAM
        sleep_ms(250);
    }

    // fill out our display info
    _dinfo.colors = 1;
    _dinfo.hres = OLED_HRES;
    _dinfo.vres = OLED_VRES;
    _dinfo.cols = DISP_CHAR_COLS;
    _dinfo.rows = DISP_CHAR_LINES;
    _dinfo.attrs = (DISP_ATTR_INVERSE | DISP_ATTR_UNDERLINE);
}

static void _write_buf(uint8_t* buf, size_t len) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one operation!
    disp_data_op_start();
    disp_write_buf(buf, len);
    disp_op_end();
}


// ///////////////////////////////////////////////////////////////////////////////////
// ////  Public Methods                                                           ////
// ///////////////////////////////////////////////////////////////////////////////////
//

void display_fill(uint8_t* buf, uint8_t fill_data) {
    // fill entire buffer with the same byte
    for (int i = 0; i < OLED_BUF_LEN; i++) {
        *(buf + i) = fill_data;
    }
};

void display_fill_page(uint8_t* buf, uint8_t fill_data, uint8_t page) {
    // fill entire page with the same byte
    memset(buf + (page * OLED_HRES), fill_data, OLED_HRES);
};

void display_render(uint8_t* buf, render_area_t* area) {
    // update a portion of the display with a render area
    uint8_t scl;
    uint8_t sch;
    uint8_t ec = area->end_col;
    uint8_t sc = area->start_col;
    uint8_t cols = (ec + sc) + 1;
    int written = 0;

    for (uint8_t p = area->start_page; p <= area->end_page && written < area->buflen; p++) {
        oled1106_send_cmd(OLED_PAGE_ADDRx | p);
        for (uint8_t c = sc; c <= ec && written < area->buflen; c += cols) {
            scl = c & 0x0F;
            sch = c >> 4;
            disp_cmd_op_start();
            disp_write(OLED_COL_ADDR_LOWx | scl);
            disp_write(OLED_COL_ADDR_HIGHx | sch);
            disp_op_end();
            disp_data_op_start();
            disp_write_buf(buf + written, cols);
            disp_op_end();
            written += cols;
        }
    }
}

/*
 * Clear the current text content and the screen.
 *
 *  \param paint Set true to paint the actual display. Otherwise, only buffers will be cleared.
*/
void display_clear(bool paint) {
    memset(display_full_screen_text, 0x00, sizeof(display_full_screen_text));
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
void display_char(unsigned short int row, unsigned short int col, const char c, bool underline, bool paint) {
    if (row >= DISP_CHAR_LINES || col >= DISP_CHAR_COLS) {
        return;  // Invalid row or column
    }
    *(display_full_screen_text + (row * DISP_CHAR_COLS) + col) = c;
    unsigned char cl = c & 0x7F;
    // Calculate the display pages the character falls into,
    // the font data shift, and the mask required.
    uint8_t pagel = (row * FONT_HEIGHT) / (OLED_PAGE_HEIGHT);
    uint16_t offsetl = pagel * OLED_HRES;
    uint16_t offseth = (pagel + 1) * OLED_HRES;
    uint8_t shift = ((FONT_HEIGHT - OLED_PAGE_HEIGHT) * row) % OLED_PAGE_HEIGHT;
    uint16_t mask = (FONT_BIT_MASK << shift) ^ 0xFFFF;
    uint16_t invert_mask = (underline ? 0x0200 : 0x0000);
    if (c & DISP_CHAR_INVERT_BIT) {
        invert_mask = ((invert_mask ^ 0x03FF) << shift);
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

const display_info_t display_info() {
    return _dinfo;
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
    memset((display_full_screen_text + (row * DISP_CHAR_COLS)), 0x00, DISP_CHAR_COLS);
    // Calculate the display pages the character falls into
    // and the mask required.
    uint8_t pagel = (row * FONT_HEIGHT) / (OLED_PAGE_HEIGHT);
    uint16_t offsetl = pagel * OLED_HRES;
    uint16_t offseth = (pagel + 1) * OLED_HRES;
    uint8_t shift = ((FONT_HEIGHT - OLED_PAGE_HEIGHT) * row) % OLED_PAGE_HEIGHT;
    uint16_t mask = (FONT_BIT_MASK << shift) ^ 0xFFFF;
    for (int i = 0; i < OLED_HRES; i++) {
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
    render_area_t row_area = {start_col: 0, end_col : OLED_HRES - 1, start_page : pagel, end_page : pagel + 1};
    _calc_render_area_buflen(&row_area);
    display_render(display_buf + (pagel * OLED_HRES), &row_area);
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
    memmove((display_full_screen_text + (row_t * DISP_CHAR_COLS)), (display_full_screen_text + ((row_t + 1) * DISP_CHAR_COLS)), (row_b - row_t) * DISP_CHAR_COLS);
    memset((display_full_screen_text + (row_b * DISP_CHAR_COLS)), 0x00, DISP_CHAR_COLS);
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
void display_string(unsigned short int row, unsigned short int col, const char *pString, bool invert, bool underline, bool paint) {
    if (row >= DISP_CHAR_LINES || col >= DISP_CHAR_COLS) {
        return;  // Invalid row or column
    }
    for (unsigned char c = *pString; c != 0; c = *(++pString)) {
        if (invert) {
            c = c ^ DISP_CHAR_INVERT_BIT;
        }
        display_char(row, col, c, underline, false);
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
            unsigned char d = *(display_full_screen_text + (r * DISP_CHAR_COLS) + c);
            display_char(r, c, d, false, false);
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

// ///////////////////////////////////////////////////////////////////////////////////
// ////  Module Initialization                                                    ////
// ///////////////////////////////////////////////////////////////////////////////////
//

/*
 * This must be called before using the display.
 */
void display_module_init(bool invert) {
    // run through the complete initialization process
    _oled1106_module_init(invert);
    display_clear(true);
}

