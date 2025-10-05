/**
 * Programmable Device operations.
 *
 * Maintains the image to be programmed and provides operations to load/modify/save it and
 * perform operations on the device to be programmed.
 *
 * Although the RP2350 has 520KB of RAM, we can't allocate a 512KB buffer to hold the largest
 * image that is supported for programming. That is because there needs to be room for a
 * stack for each of the cores, room for interrupt service tables, and other data that is
 * used by the code. Therefore, a 256KB buffer is allocated (so 128KB and 256KB devices can
 * be programmed without issue), and if a 512KB device is to be programmed, an SDCard must
 * be available, and the SDCard is used to contain a 'temp' file that holds the image while
 * operations are performed on the device.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "prog_device.h"
#include "pdops.h"

#include "board.h"
#include "cmt/cmt_t.h"
#include "util/util.h"

#include "pico/types.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>  // For memset()

// ====================================================================
// Data Section
// ====================================================================

static bool _initialized;

#define MT_BYTE_VAL 0xFF
#define PROG_DEV_BUF_SIZE (8*ONE_K)
/** @brief Image for the programmable device (512MB) */
static uint8_t _imgbuf[PROG_DEV_BUF_SIZE];

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

void _clr_device_buf() {
    memset(_imgbuf, MT_BYTE_VAL, PROG_DEV_BUF_SIZE);
}

// ====================================================================
// Public Methods
// ====================================================================


// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================

void pd_module_init() {
    if (_initialized) {
        board_panic("!!! pd_module_init: Called more than once !!!");
    }
    _clr_device_buf();
    pdo_module_init();
}
