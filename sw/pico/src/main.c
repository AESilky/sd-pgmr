/**
 * SD Flash Programmer main application.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "system_defs.h" // Main system/board/application definitions
//
#include "board.h"
#include "debug_support.h"
#include "multicore.h"
#include "picoutil.h"
//
#include "cmt.h"
#include "hwrt/hwrt.h"
//
#include "pico/binary_info.h"

#include <stdio.h>


#define DOT_MS 60 // Dot at 20 WPM
#define UP_MS  DOT_MS
#define DASH_MS (2 * DOT_MS)
#define CHR_SP (3 * DOT_MS)

 // 'H' (....) 'I' (..)
static const int32_t say_hi[] = {
    DOT_MS,
    UP_MS,
    DOT_MS,
    UP_MS,
    DOT_MS,
    UP_MS,
    DOT_MS,
    CHR_SP,
    DOT_MS,
    UP_MS,
    DOT_MS,
    500, // Pause before repeating
    0 };

int main() {
    // useful information for picotool
    bi_decl(bi_program_description("SilkyDESIGN Flash Programmer"));

    // Initialize debug
    debug_init(DIM_BOOT);

    // Board/base level initialization
    if (board_init() != 0) {
        board_panic("Board init failed.");
    }

    led_on_off(say_hi);

    sleep_ms(800);

    // Initialize the multicore subsystem
    multicore_minit(debug_mode_enabled());

    // Initialize the Cooperative Multi-Tasking subsystem
    cmt_minit();

    // Starting Core-1 will run the `core1_main` which is defined for the appropriate
    // Board-0 or Board-1 functionality.
    start_core1();

    // Launch the Hardware Runtime (core-0 (endless) Message Dispatching Loop).
    // The HWRT starts the appropriate secondary operations (core-1 message loop)
    // (!!! THIS NEVER RETURNS !!!)
    start_hwrt();

    // How did we get here?!
    error_printf("SDPGMR.main - Somehow we are out of our endless message loop in `main()`!!!");
    // ZZZ Reboot!!!
    return 0;
}
