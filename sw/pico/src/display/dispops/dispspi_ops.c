/**
 * Copyright 2023 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "dispops.h"
#include "system_defs.h"
#include "board.h"

#include "hardware/spi.h"

/** @brief Board Operation Token, needed to perform Disp C/D select */
static boptkn_t boptkn;

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
        // Assert the Display-Control signal
        board_op(boptkn, BDO_DISPLAY_CTRL);
    }
    else {
        // Assure that the Display-Control signal is not asserted
        board_op(boptkn, BDO_NONE);
    }
}

bool disp_cmd_op_start() {
    boptkn = board_op_start();
    if (boptkn == NULL) {
        return false;
    }
    _command_mode(true);
    _cs(true);
    return true;
}

bool disp_data_op_start() {
    boptkn = board_op_start();
    if (boptkn == NULL) {
        return false;
    }
    _command_mode(false);
    _cs(true);
    return true;
}

void disp_op_end() {
    _cs(false);
    board_op_end(boptkn);
}

void disp_reset() {
    boptkn = board_op_start();
    if (boptkn == NULL) {
        return;
    }
    board_op(boptkn, BDO_NONE);
    sleep_ms(10);
    board_op(boptkn, BDO_DISPLAY_RST);
    sleep_ms(10);
    board_op(boptkn, BDO_NONE);
    board_op_end(boptkn);
}

int disp_write(uint8_t data) {
    return (spi_write_blocking(SPI_SD_DISP_DEVICE, &data, 1));
}

int disp_write_buf(const uint8_t* data, size_t len) {
    return (spi_write_blocking(SPI_SD_DISP_DEVICE, data, len));
}
