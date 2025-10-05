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


void pdo_addr_inc() {
    boptkn_t tkn = board_op_start();
    board_op(tkn, BDO_ADDR_CLK);    // This takes the clock low on the counters
    sleep_ms(1);                    // Technically not needed, but it makes us feel better
    board_op(tkn, BDO_NONE);        // This takes the clock back high, which actually advances the count
    board_op_end(tkn);
}

void pdo_addr_set(uint32_t addr) {
    uint8_t addrL = addr & 0x000000FF;
    uint8_t addrM = (addr & 0x0000FF00) >> 8;
    uint8_t addrH = (addr & 0x000F0000) >> 16;
    // Write out each of the address parts to the appropriate latch/counter
    // This requires three Board Ops, so we start an Op and keep it for all three.
    boptkn_t tkn = board_op_start();
    pdatabus_wr(addrL);
    sleep_ms(1);                    // Technically not needed, but it makes us feel better
    board_op(tkn, BDO_ADDR_LOW_LD); // This takes the Latch/Counter LD low (data at IN goes to OUT)
    sleep_ms(1);                    // Technically not needed, but it makes us feel better
    board_op(tkn, BDO_NONE);        // This takes the Latch/Counter LD high (latches the value in the counter)
    pdatabus_wr(addrM);
    sleep_ms(1);
    board_op(tkn, BDO_ADDR_MID_LD);
    sleep_ms(1);
    board_op(tkn, BDO_NONE);
    pdatabus_wr(addrL);
    sleep_ms(1);
    board_op(tkn, BDO_ADDR_LOW_LD);
    sleep_ms(1);
    board_op(tkn, BDO_NONE);
    board_op_end(tkn);
}

void pdo_wr_en(bool enable) {
    // Ensure that FRD- is not asserted if FWR- is being asserted.
    //  (signals are active low)
    if (enable) {
        gpio_put(OP_DATA_RD, 1);
    }
    gpio_put(OP_DEVICE_WR, !enable);
}

void pdo_module_init() {
    pdo_pwr_on(false);
}
