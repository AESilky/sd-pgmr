/**
 * Programmable Device Operations (low-level).
 *
 * Methods to control the POWER-ENABLE and READ/WRITE-ENABLE signals, and to load the
 * address into the address latches/counters.
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

typedef enum _frdwrbits {
    _FRW_NONE   = 0x38,
    _FRD        = 0x80,
    _FWR        = 0x40,
} _frdwrb_t;
#define _FRDWR_MASK 0xC0

// ====================================================================
// Data Section
// ====================================================================

static volatile bool _initialized;

static bool _op_ip;
static boptkn_t _tkn;
/** Holds the top 3-bits of the address and the FWR- and FRD- control bits. */
static uint8_t _addrHctrl;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================

/**
 * @brief Select/deselect the device. Board-Op must be inprogress.
 *
 * @param cs True: Select Device
 */
static void _cs(bool cs) {
    if (cs) {
        board_op(_tkn, BDO_PRGMDEV_SEL);
    }
    else {
        board_op(_tkn, BDO_NONE);
    }
}

static void _op_end() {
    if (_op_ip) {
        board_op_end(_tkn);
        _op_ip = false;
    }
}

static void _op_start() {
    if (!_op_ip) {
        _tkn = board_op_start();
        _op_ip = true;
    }
}

/**
 * @brief Set the given RD&WR bits into the AddrH+Ctrl and write it to the latch.
 * This must be called from within a Board-OP.
 *
 * This sets the specified bit LOW and the other HIGH.
 *
 * @param rwbits RD&WR bits to set low.
 */
static void _pd_rw_set(_frdwrb_t rwbits) {
    _addrHctrl = (_addrHctrl & ~_FRDWR_MASK) | (~rwbits & _FRDWR_MASK);
    board_op(_tkn, BDO_ADDR_HIGH_LD);   // This takes the Latch CLK low
    pdatabus_wr(_addrHctrl);
    sleep_us(2);
    board_op(_tkn, BDO_NONE);           // This takes the Latch CLK high (clocks data to the output)
}

void pdo_addr_set(uint32_t addr) {
    uint8_t addrL = addr & 0x000000FF;
    uint8_t addrM = (addr & 0x0000FF00) >> 8;
    uint8_t addrH = ((addr & 0x000F0000) >> 16);
    _addrHctrl = (_addrHctrl & _FRDWR_MASK) | addrH; // Merge the address and the RD/WR ctrl bits

    // Write out each of the address parts to the appropriate latch/counter
    // This requires three Board Ops, so we start an Op and keep it for all three.
    _op_start();
    board_op(_tkn, BDO_ADDR_HIGH_LD);   // This takes the Latch CLK low
    pdatabus_wr(_addrHctrl);            // Put the data on the bus
    sleep_us(2);                        // Technically not needed, but it makes us feel better
    board_op(_tkn, BDO_ADDR_MID_LD);    // This takes the previous Latch CLK high and this one low
    pdatabus_wr(addrM);
    sleep_us(2);
    board_op(_tkn, BDO_ADDR_LOW_LD);
    pdatabus_wr(addrL);
    sleep_us(2);
    board_op(_tkn, BDO_NONE);           // Take the last CLK high
    // Put the P Data Bus back to input
    pdatabus_set_in();
    _op_end();
}

uint8_t pdo_data_get() {
    _op_start();
    _pd_rw_set(_FRD);
    _cs(true);
    // Take 'data_latch' low then high
    sleep_us(2);
    gpio_put(OP_DATA_LATCH, 0);
    sleep_us(2);
    gpio_put(OP_DATA_LATCH, 1);
    _cs(false);
    _pd_rw_set(_FRW_NONE);
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
    sleep_us(2);
    gpio_put(OP_DATA_LATCH, 1);
    // Enable the data latch output
    gpio_put(OP_DATA_WR, 0);
    // Set /WR on the device
    _pd_rw_set(_FWR);
    // Select the device
    _cs(true);
    sleep_us(2);
    _cs(false);
    _pd_rw_set(_FRW_NONE);
    // Disable the data latch output
    gpio_put(OP_DATA_WR, 1);
    _op_end();
}


void pdo_minit() {
    if (_initialized) {
        board_panic("!!! pdo_module_init called multiple times !!!");
    }
    _initialized = true;

    pdo_pwr_on(false);
    _addrHctrl = (~_FRW_NONE);
    pdo_addr_set(0);
}
