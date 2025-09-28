/**
 * Copyright 2023 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "oled1106_spi.h"
#include "../display_spi/display_spi_ops.h"

#include <stdio.h>

/*! @brief Memory area for the screen data pixel-bytes */
uint8_t display_buf[OLED_BUF_LEN];

/*! @brief Memory area for the screen data pixel-bytes */
render_area_t display_full_area = {start_col: 0, end_col : OLED_WIDTH - 1, start_page : 0, end_page : OLED_NUM_PAGES - 1};

static void _oled_send_cmd(uint8_t cmd) {
    display_op_start(DISP_OP_CMD);
    display_data_write(cmd);
    display_op_end();
}

static void _oled_send_cmdx(uint8_t* cmds, size_t num) {
    display_op_start(DISP_OP_CMD);
    for (size_t i = 0; i < num; i++) {
        display_data_write(*(cmds + i));
    }
    display_op_end();
}


void oled_display_clear() {
    uint16_t p, w;
    for (p = 0; p < OLED_NUM_PAGES; p++) {
        /* set page address */
        _oled_send_cmd(OLED_PAGE_ADDRx | p);
        _oled_send_cmd(OLED_COL_ADDR_LOWx | 0x00);
        _oled_send_cmd(OLED_COL_ADDR_HIGHx | 0x00);
        display_op_start(DISP_OP_DATA);
        for (w = 0; w < OLED_WIDTH; w++) {
            display_data_write(0x00);
        }
        display_op_end();
    }
}

void display_fill(uint8_t *buf, uint8_t fill_data) {
    // fill entire buffer with the same byte
    for (int i = 0; i < OLED_BUF_LEN; i++) {
        *(buf + i) = fill_data;
    }
};

void display_fill_page(uint8_t *buf, uint8_t fill_data, uint8_t page) {
    // fill entire page with the same byte
    memset(buf + (page * OLED_WIDTH), fill_data, OLED_WIDTH);
};

void calc_render_area_buflen(render_area_t *area) {
    // calculate how long the flattened buffer will be for a render area
    area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
}

void oled_write_buf(uint8_t *buf, size_t len) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one operation!
    display_op_start(DISP_OP_DATA);
    display_write_buf(buf, len);
    display_op_end();
}

void oled_module_init() {
    // Some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are all included
    // rather than rely on POR.

    // some configuration values are recommended by the board manufacturer

    // We must reset the display for it to function
    display_reset();

    _oled_send_cmd(OLED_DISP_OFF_ONx | 0x00);     // set display off

    _oled_send_cmd(OLED_COL_ADDR_LOWx | 0x00);    // set the column address
    _oled_send_cmd(OLED_COL_ADDR_HIGHx | 0x00);

    _oled_send_cmd(OLED_DISP_START_LINEx | 0x00); // start at 0

    _oled_send_cmd(OLED_CONTRAST);
    _oled_send_cmd(0x80);                        // Mid contrast to start

    _oled_send_cmd(OLED_SEG_COL_MAPx | 0x00);    // Normal segment/column mapping
    _oled_send_cmd(OLED_COM_ROW_DIRx | 0x00);    // Scan from 0 to n
    _oled_send_cmd(OLED_NORM_INVx | 0x00);       // Set normal

    _oled_send_cmd(OLED_MUX_RATIO);
    _oled_send_cmd(0x3F);                        // 1/64 ratio

    _oled_send_cmd(OLED_DISP_OFFSET);
    _oled_send_cmd(0x00);                        // No offset

    _oled_send_cmd(OLED_DISP_CLK_DIV);
    _oled_send_cmd(0x80);                        // (xxxx yyyy) x=+15% y=f/1 (100fps)

    _oled_send_cmd(OLED_PRECHARGE);
    _oled_send_cmd(0xF1);                        // Dis-Charge 15 Clocks & Pre-Charge 1 Clock

    _oled_send_cmd(OLED_COM_PIN_CFG);
    _oled_send_cmd(0x12);                        // (000D 0010) D=1 Alternative

    _oled_send_cmd(OLED_VCOM_DESEL);
    _oled_send_cmd(0x40);                        // Beta=1

    _oled_send_cmd(OLED_PAGE_ADDRx | 0x00);      // Page 0
    _oled_send_cmd(OLED_NORM_INVx | 0x00);       // Normal display

    // initialize render area for entire display (128 pixels by 8 pages)
    calc_render_area_buflen(&display_full_area);

    // zero the entire display
    oled_display_clear();
    display_fill(display_buf, 0x00);
    display_render(display_buf, &display_full_area);

    _oled_send_cmd(OLED_DISP_OFF_ONx | 0x01);    // Turn display on

    // intro sequence: flash the screen twice
    for (int i = 0; i < 2; i++) {
        _oled_send_cmd(OLED_ENTIRE_ONx | 0x01); // ignore RAM, all pixels on
        sleep_ms(250);
        _oled_send_cmd(OLED_ENTIRE_ONx | 0x00); // go back to following RAM
        sleep_ms(250);
    }
}

void display_render(uint8_t *buf, render_area_t *area) {
    // update a portion of the display with a render area
    uint8_t scl;
    uint8_t sch;
    uint8_t ec = area->end_col;
    uint8_t sc = area->start_col;
    uint8_t cols = (ec + sc) + 1;
    int written = 0;

    for (uint8_t p = area->start_page; p <= area->end_page && written < area->buflen; p++) {
        _oled_send_cmd(OLED_PAGE_ADDRx | p);
        for (uint8_t c = sc; c <= ec && written < area->buflen; c+=cols) {
            scl = c & 0x0F;
            sch = c >> 4;
            display_op_start(DISP_OP_CMD);
            display_data_write(OLED_COL_ADDR_LOWx | scl);
            display_data_write(OLED_COL_ADDR_HIGHx | sch);
            display_op_end();
            display_op_start(DISP_OP_DATA);
            display_write_buf(buf+written, cols);
            display_op_end();
            written += cols;
        }
    }
}
