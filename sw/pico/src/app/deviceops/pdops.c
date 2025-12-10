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
#include "dbus.h"
#include "system_defs.h"
#include "debug_support.h"

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

/** The current Power Mode */
static progdev_pwr_mode_t _pwrmode;
/** Holds the top 3-bits of the address and the FWR- and FRD- control bits. */
static uint8_t _addrHctrl;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================


// ====================================================================
// Local/Private Method Definitions
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

static bool _pd_pwr_chk() {
    if (!pdo_pwr_is_on()) {
        if (_pwrmode == PDPWR_OFF) {
            warn_printf("PD Operations require that the Programmable Device is powered!\n");
            return (false);
        }
        else {
            // If the power is off and the mode allows, turn it on.
            return (pdo_request_pwr_on(true));
        }
    }
    return (true);
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
    dbus_wr(_addrHctrl);
    sleep_us(2);
    board_op(_tkn, BDO_NONE);           // This takes the Latch CLK high (clocks data to the output)
}


// ====================================================================
// Public Methods
// ====================================================================

void pdo_addr_set(uint32_t addr) {
    if (!_pd_pwr_chk()) {
        ERRORNO = -1;
        return;
    }

    uint8_t addrL = addr & 0x000000FF;
    uint8_t addrM = (addr & 0x0000FF00) >> 8;
    uint8_t addrH = ((addr & 0x000F0000) >> 16);
    _addrHctrl = (_addrHctrl & _FRDWR_MASK) | addrH; // Merge the address and the RD/WR ctrl bits

    // Write out each of the address parts to the appropriate latch/counter
    // This requires three Board Ops, so we start an Op and keep it for all three.
    _op_start();
    board_op(_tkn, BDO_ADDR_HIGH_LD);   // This takes the Latch CLK low
    dbus_wr(_addrHctrl);            // Put the data on the bus
    sleep_us(2);                        // Technically not needed, but it makes us feel better
    board_op(_tkn, BDO_ADDR_MID_LD);    // This takes the previous Latch CLK high and this one low
    dbus_wr(addrM);
    sleep_us(2);
    board_op(_tkn, BDO_ADDR_LOW_LD);
    dbus_wr(addrL);
    sleep_us(2);
    board_op(_tkn, BDO_NONE);           // Take the last CLK high
    // Put the P Data Bus back to input
    dbus_set_in();
    _op_end();
}

uint8_t pdo_data_get() {
    if (!_pd_pwr_chk()) {
        ERRORNO = -1;
        return (-1);
    }

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
    uint8_t data = dbus_rd();
    // Disable the output of the data-in latch
    gpio_put(OP_DATA_RD, 1);
    _op_end();

    return (data);
}

uint8_t pdo_data_get_from(uint32_t addr) {
    ERRORNO = 0;
    pdo_addr_set(addr);
    if (ERRORNO < 0) {
        return (-1);
    }
    uint8_t v = pdo_data_get();
    if (ERRORNO < 0) {
        return (-1);
    }
    return (v);
}

void pdo_data_set(uint8_t data) {
    if (!_pd_pwr_chk()) {
        ERRORNO = -1;
        return;
    }

    _op_end(); // Just in case something left the device in a command state
    _op_start();
    dbus_wr(data); // put the data into the output latch
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

void pdo_data_set_at(uint32_t addr, uint8_t data) {
    ERRORNO = 0;
    pdo_addr_set(addr);
    if (ERRORNO < 0) {
        return;
    }
    pdo_data_set(data);
    return;
}

void pdo_pwr_mode(progdev_pwr_mode_t mode) {
    _pwrmode = mode;
    switch (_pwrmode) {
        case PDPWR_OFF:
            pdo_request_pwr_on(false);
            break;
        case PDPWR_ON:
            pdo_request_pwr_on(true);
            break;
        case PDPWR_AUTO:
            // Typically keep the power off
            pdo_request_pwr_on(false);
            break;
    }
}

progdev_pwr_mode_t pdo_pwr_mode_get() {
    return (_pwrmode);
}

bool pdo_request_pwr_on(bool on) {
    static bool _1st_pon;
    if (on == pdo_pwr_is_on()) {
        return (true);
    }
    bool retval = false;
    if (_pwrmode == PDPWR_AUTO || ((_pwrmode == PDPWR_ON && on) || (_pwrmode == PDPWR_OFF && !on))) {
        if (!on) {
            // Set LOW to avoid back-powering circuit
            gpio_put(OP_DATA_WR, 0);
            gpio_put(OP_DATA_LATCH, 0);
            // Set DATA to 0
            dbus_wr(0);
            // Set DataBus IN
            dbus_set_in();
        }
        gpio_put(OP_DEVICE_PWR, on);
        if (on) {
            gpio_put(OP_DATA_WR, 1); // Set HIGH to avoid driving the PD Data Bus
            // Leave the DATA_LATCH, as taking it from LOW to HIGH latches data
            sleep_ms(5); // Allow the device to have power for a few ms before access
            if (!_1st_pon) {
                // This is our first time powering the device on.
                _1st_pon = true;
                // Do a single byte read, to flush garbage.
                pdo_addr_set(0);
                uint8_t d = pdo_data_get();
                debug_printf("First device read: %2X\n", d);
            }
        }
        retval = true;
    }

    return (retval);
}



void pdo_minit() {
    if (_initialized) {
        board_panic("!!! pdo_module_init called multiple times !!!");
    }
    _initialized = true;

    pdo_pwr_mode(PDPWR_AUTO);
    _addrHctrl = (~_FRW_NONE);
}
