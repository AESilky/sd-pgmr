/**
 * Disk Operations.
 *
 * This provides a higher-level interface to the SD Card and FATFS libraries (3rd-Party).
 *
 * Since the SD Card and FATFS are 3rd-Party libraries, this modules centralizes the
 * use of them so that they can be more easily replaced if so desired (a problem is
 * found with them or a 'better' library is found).
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "dskops.h"

#include "board.h"
#include "cmt/cmt_t.h"
#include "sd_card.h"

#include "pico/types.h" // 'uint' and other standard types

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// ====================================================================
// Data Section
// ====================================================================

static bool _initialized;

static FATFS _fs;
static char* _drive;

/**
 * @brief Shared/common buffer to hold file name/path values.
 */
static char _filepath[MAX_PATH+1]; // allow for a NULL terminator

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

char* dsk_get_shared_path_buf() {
    *_filepath = '\0';

    return _filepath;
}

FRESULT dsk_mount_sd() {
    FRESULT res = FR_OK;

    if (_fs.fs_type == 0) {
        res = f_mount(&_fs, _drive, 1);
        if (FR_OK != res) {
            error_printf(false, "Could not mount SD: (Error: %d)\n", res);
        }
    }

    return (res);
}

FRESULT dsk_unmount_sd() {
    FRESULT res = FR_OK;

    if (_fs.fs_type != 0) {
        res = f_unmount(_drive);
        _fs.fs_type = 0;
    }

    return (res);
}


// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================

void dskops_module_init() {
    if (_initialized) {
        board_panic("!!! dskops_module_init: Called more than once !!!");
    }
    sd_init_driver();
    _drive = "0:";
    _fs.fs_type = 0;

    _initialized = true;
}
