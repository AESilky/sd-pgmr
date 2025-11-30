/**
 * Copyright 2023-25 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "dispops.h"
#include "system_defs.h"
#include "board.h"

#include "hardware/spi.h"

#define DISP_CMD_ENABLE  0  // Low signal is CMD
#define DISP_DATA_ENABLE 1  // High signal is DATA

/**
 * Set the chip select for the display.
 *
 */
static void _cs(bool sel) {
    if (sel) {
        gpio_put(SPI_DISPLAY_CS, SPI_CS_ENABLE);
    }
    else {
        gpio_put(SPI_DISPLAY_CS, SPI_CS_DISABLE);
    }
}

/**
 * Set the data/command select for the display.
 *
 */
static void _command_mode(bool cmd) {
    if (cmd) {
        gpio_put(SPI_DISPLAY_CTRL, DISP_CMD_ENABLE);
    }
    else {
        gpio_put(SPI_DISPLAY_CTRL, DISP_DATA_ENABLE);
   }
}

bool disp_cmd_op_start() {
    _command_mode(true);
    _cs(true);
    return true;
}

bool disp_data_op_start() {
    _command_mode(false);
    _cs(true);
    return true;
}

void disp_op_end() {
    _command_mode(false);
    _cs(false);
}

void disp_reset() {
    gpio_put(DISPLAY_RST, 0);
    sleep_ms(8);
    gpio_put(DISPLAY_RST, 1);
}

int disp_write(uint8_t data) {
    return (spi_write_blocking(SPI_SD_DISP_DEVICE, &data, 1));
}

int disp_write_buf(const uint8_t* data, size_t len) {
    return (spi_write_blocking(SPI_SD_DISP_DEVICE, data, len));
}
