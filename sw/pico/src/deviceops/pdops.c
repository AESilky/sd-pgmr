/**
 * Programmable Device Operations (low-level).
 *
 * Methods to control the POWER-ENABLE and WRITE-ENABLE signals, to load the
 * address into the address latches/counters, and advance the address.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "pdops.h"

#include "board.h"
#include "system_defs.h"

#include "pico/stdlib.h"
#include "pico/types.h"

// ====================================================================
// Data Section
// ====================================================================

static volatile bool _initialized;

static bool _op_ip;
static boptkn_t _tkn;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================

/**
 * @brief Select/deselect the device. Board-Op must be inprogress.
 *
 * @param cs True: Select Device
 */
void _cs(bool cs) {
    if (cs) {
        board_op(_tkn, BDO_PRGMDEV_SEL);
    }
    else {
        board_op(_tkn, BDO_NONE);
    }
}

void _op_end() {
    if (_op_ip) {
        board_op_end(_tkn);
        _op_ip = false;
    }
}

void _op_start() {
    if (!_op_ip) {
        _tkn = board_op_start();
        _op_ip = true;
    }
}

void _rd_en(bool enable) {
    // Ensure that FWR- is not asserted if FRD- is being asserted.
    //  (signals are active low)
    if (enable) {
        gpio_put(OP_DEVICE_WR, 1);
    }
    gpio_put(OP_DATA_RD, !enable);
}

void _wr_en(bool enable) {
    // Ensure that FRD- is not asserted if FWR- is being asserted.
    //  (signals are active low)
    if (enable) {
        gpio_put(OP_DATA_RD, 1);
    }
    gpio_put(OP_DEVICE_WR, !enable);
}



void pdo_addr_inc() {
    _op_start();
    board_op(_tkn, BDO_ADDR_CLK);    // This takes the clock low on the counters
    sleep_ms(1);                    // Technically not needed, but it makes us feel better
    board_op(_tkn, BDO_NONE);        // This takes the clock back high, which actually advances the count
    _op_end();
}

void pdo_addr_set(uint32_t addr) {
    uint8_t addrL = addr & 0x000000FF;
    uint8_t addrM = (addr & 0x0000FF00) >> 8;
    uint8_t addrH = (addr & 0x000F0000) >> 16;
    // Write out each of the address parts to the appropriate latch/counter
    // This requires three Board Ops, so we start an Op and keep it for all three.
    _op_start();
    pdatabus_wr(addrL);
    sleep_ms(1);                    // Technically not needed, but it makes us feel better
    board_op(_tkn, BDO_ADDR_LOW_LD); // This takes the Latch/Counter LD low (data at IN goes to OUT)
    sleep_ms(1);                    // Technically not needed, but it makes us feel better
    board_op(_tkn, BDO_NONE);        // This takes the Latch/Counter LD high (latches the value in the counter)
    pdatabus_wr(addrM);
    sleep_ms(1);
    board_op(_tkn, BDO_ADDR_MID_LD);
    sleep_ms(1);
    board_op(_tkn, BDO_NONE);
    pdatabus_wr(addrH);
    sleep_ms(1);
    board_op(_tkn, BDO_ADDR_HIGH_LD);
    sleep_ms(1);
    board_op(_tkn, BDO_NONE);
    _op_end();
}

uint8_t pdo_data_get() {
    _op_start();
    _wr_en(false); // Assure WR isn't enabled before we select the device
    _cs(true);
    _rd_en(true); // Output enable on device
    // Take 'data_latch' low then high
    gpio_put(OP_DATA_LATCH, 0);
    sleep_ms(1);
    gpio_put(OP_DATA_LATCH, 1);
    _cs(false);
    _rd_en(false);
    // Enable the output of the data-in latch
    gpio_put(OP_DATA_RD, 0);
    // Read the data-in latch
    uint8_t data = pdatabus_rd();
    // Disable the output of the data-in latch
    gpio_put(OP_DATA_RD, 1);
    _op_end();

    return (data);
}

void pdo_data_set(uint8_t data) {
    _op_start();
    pdatabus_wr(data); // put the data into the output latch
    // Take 'data_latch' low then high
    gpio_put(OP_DATA_LATCH, 0);
    sleep_ms(1);
    gpio_put(OP_DATA_LATCH, 1);
    // Enable the data latch output
    gpio_put(OP_DATA_WR, 0);
    // Select the device
    _cs(true);
    _wr_en(true); // Write enable on device
    sleep_ms(1);
    _wr_en(false);
    _cs(false);
    _op_end();
    // Disable the data latch output
    gpio_put(OP_DATA_WR, 1);
}


void pdo_module_init() {
    if (_initialized) {
        board_panic("!!! pdo_module_init called multiple times !!!");
    }
    _initialized = true;

    pdo_pwr_on(false);
    _wr_en(false);
}
