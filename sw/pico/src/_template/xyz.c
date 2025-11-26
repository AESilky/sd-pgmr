/**
 * XYZ Template.
 *
 * [ZZZ Replace This]
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */

#include "xyz.h"

#include "cmt/cmt_t.h"

#include "pico/types.h" // 'uint' and other standard types

#include <stdbool.h>
#include <stdint.h>
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

/**
 * @brief Called after delay.
 *
 * This has been delayed.
 *
 * @param data Nothing important (can be pointer to anything needed)
 */
static void _delay_action(void* data) {
}


// ====================================================================
// Message Handler Methods
// ====================================================================

/**
 * @brief Handle XYZ Housekeeping tasks. This is triggered every ~16ms.
 *
 * For reference, 625 times is 10 seconds.
 *
 * @param msg Nothing important in the message.
 */
static void _handle_housekeeping(cmt_msg_t* msg) {
    static uint cnt = 0;

    cnt++;
}


// ====================================================================
// Local/Private Methods
// ====================================================================


// ====================================================================
// Public Methods
// ====================================================================


// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================


void xyz_minit() {
    if (_initialized) {
        board_panic("!!! xyz_module_init: Called more than once !!!");
    }
}
