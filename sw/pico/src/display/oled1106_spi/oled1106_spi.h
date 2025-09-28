/**
 * Copyright 2023 AESilky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _OLED1106_SPI_H_
#define _OLED1106_SPI_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"

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
#define OLED_SEG_COL_MAPx 0xA0      // A0=Norml A1=Reverse
#define OLED_MUX_RATIO 0xA8         // Next byte (0-63) is ratio
#define OLED_COM_ROW_DIRx 0xC0      // (1100 D000) Scan D=0 0-n, D=1 n-0
#define OLED_DISP_OFFSET 0xD3       // Next byte (0-63) is offset
#define OLED_COM_PIN_CFG 0xDA       // Next byte controls common pads
#define OLED_DISP_CLK_DIV 0xD5      // Next byte controls clock divider (see datasheet)
#define OLED_PRECHARGE 0xD9         // Next byte controls precharge (Dis/Pre)
#define OLED_VCOM_DESEL 0xDB        // Next byte controls VCOM deselect level
#define OLED_CHARGE_PUMPx 0x32      // (0011 00xx) DC-DC output voltage
//
#define OLED_HEIGHT 64
#define OLED_WIDTH 132
#define OLED_DEAD_LEFT 2
#define OLED_PAGE_HEIGHT 8
#define OLED_NUM_PAGES OLED_HEIGHT / OLED_PAGE_HEIGHT
#define OLED_BUF_LEN (OLED_NUM_PAGES * OLED_WIDTH)
//

#include "../display.h"

extern void calc_render_area_buflen(render_area_t* area);

void oled_module_init();

#ifdef __cplusplus
}
#endif
#endif // _OLED1106_SPI_H_
