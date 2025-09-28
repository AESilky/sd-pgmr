/**
 * Copyright 2023 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "display_spi_ops.h"
#include "system_defs.h"
#include "board.h"

#include "hardware/spi.h"

#define DISPLAY_DC_CMD 0
#define DISPLAY_DC_DATA 1

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

bool display_op_start(op_cmd_data_t cd) {
    boptkn = board_op_start();
    if (boptkn == NULL) {
        return false;
    }
    if (cd == DISP_OP_CMD) {
        _command_mode(true);
    }
    else {
        _command_mode(false);
    }
    _cs(true);
    return true;
}

void display_op_end() {
    _cs(false);
    board_op_end(boptkn);
}

void display_reset() {
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

int display_data_write(uint8_t data) {
    return (spi_write_blocking(SPI_SD_DISP_DEVICE, &data, 1));
}

int display_write_buf(const uint8_t* data, size_t len) {
    return (spi_write_blocking(SPI_SD_DISP_DEVICE, data, len));
}
