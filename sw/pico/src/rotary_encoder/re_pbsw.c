/**
 * @brief Push button switch on the rotary encoder.
 * @ingroup ui
 *
 * This provides input from the momentary action push-button switch on the rotary encoder.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#include "system_defs.h"
#include "re_pbsw.h"

#include "board.h"
#include "cmt/cmt.h"

#include <stdio.h>

// ====================================================================
// Data Section
// ====================================================================

static volatile bool _initialized;


// ====================================================================
// Local/Private Method Declarations
// ====================================================================


// ====================================================================
// Message handler functions
// ====================================================================


// ====================================================================
// Local/Private Methods
// ====================================================================


// ====================================================================
// Public Methods
// ====================================================================


// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================


void re_pbsw_module_init() {
    // GPIO is initialized in `board.c` with the rest of the board.
    if (_initialized) {
        board_panic("!!! re_pbsw_module_init: Called more than once !!!");
    }
    _initialized = true;
}
