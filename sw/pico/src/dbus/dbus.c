/**
 * DBUS - Databus Operations.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */

#include "dbus.h"

#include "board.h"


#include <stddef.h>

// ====================================================================
// Data Section
// ====================================================================

static volatile bool _initialized;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================


// ====================================================================
// Run-After/Delay/Sleep Methods
// ====================================================================


// ====================================================================
// Message Handler Methods
// ====================================================================


// ====================================================================
// Local/Private Methods
// ====================================================================


// ====================================================================
// Public Methods
// ====================================================================

extern uint8_t dbus_rd() {
    if (dbus_is_out()) {
        dbus_set_in();
    }
    uint32_t rawvalue = gpio_get_all();
    uint8_t value = (rawvalue & DATA_BUS_MASK) >> DATA_BUS_SHIFT;

    return value;
}

void dbus_wr(uint8_t data) {
    dbus_set_out();
    uint32_t bdval = data << DATA_BUS_SHIFT;
    gpio_put_masked(DATA_BUS_MASK, bdval);
}



// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================


void dbus_minit() {
    if (_initialized) {
        board_panic("!!! dbus_minit: Called more than once !!!");
    }
    // Data RD, WR, and DataLatch
    gpio_set_function(OP_DATA_RD, GPIO_FUNC_SIO);
    gpio_put(OP_DATA_RD, 1);
    gpio_set_dir(OP_DATA_RD, GPIO_OUT);
    gpio_set_drive_strength(OP_DATA_RD, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(OP_DATA_WR, GPIO_FUNC_SIO);
    gpio_put(OP_DATA_WR, 0);    // Keep this pin LOW when the Programmable Device isn't powered
    gpio_set_dir(OP_DATA_WR, GPIO_OUT);
    gpio_set_drive_strength(OP_DATA_WR, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(OP_DATA_LATCH, GPIO_FUNC_SIO);
    gpio_put(OP_DATA_LATCH, 0);
    gpio_set_dir(OP_DATA_LATCH, GPIO_OUT);
    gpio_set_drive_strength(OP_DATA_LATCH, GPIO_DRIVE_STRENGTH_2MA);
    //  Data Bus (Initially set to input)
    gpio_set_function(DATA0, GPIO_FUNC_SIO);
    gpio_set_dir(DATA0, GPIO_IN);
    gpio_set_pulls(DATA0, false, true); // Pull-Down
    gpio_set_drive_strength(DATA0, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_function(DATA1, GPIO_FUNC_SIO);
    gpio_set_dir(DATA1, GPIO_IN);
    gpio_set_pulls(DATA1, false, true); // Pull-Down
    gpio_set_drive_strength(DATA1, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_function(DATA2, GPIO_FUNC_SIO);
    gpio_set_dir(DATA2, GPIO_IN);
    gpio_set_pulls(DATA2, false, true); // Pull-Down
    gpio_set_drive_strength(DATA2, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_function(DATA3, GPIO_FUNC_SIO);
    gpio_set_dir(DATA3, GPIO_IN);
    gpio_set_pulls(DATA3, false, true); // Pull-Down
    gpio_set_drive_strength(DATA3, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_function(DATA4, GPIO_FUNC_SIO);
    gpio_set_dir(DATA4, GPIO_IN);
    gpio_set_pulls(DATA4, false, true); // Pull-Down
    gpio_set_drive_strength(DATA4, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_function(DATA5, GPIO_FUNC_SIO);
    gpio_set_dir(DATA5, GPIO_IN);
    gpio_set_pulls(DATA5, false, true); // Pull-Down
    gpio_set_drive_strength(DATA5, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_function(DATA6, GPIO_FUNC_SIO);
    gpio_set_dir(DATA6, GPIO_IN);
    gpio_set_pulls(DATA6, false, true); // Pull-Down
    gpio_set_drive_strength(DATA6, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_function(DATA7, GPIO_FUNC_SIO);
    gpio_set_dir(DATA7, GPIO_IN);
    gpio_set_pulls(DATA7, false, true); // Pull-Down
    gpio_set_drive_strength(DATA7, GPIO_DRIVE_STRENGTH_4MA);

}
