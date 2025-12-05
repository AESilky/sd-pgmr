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
#include "msgpost.h"
#include "include/util.h"

#include "pico/types.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>  // For memset()


#define F_CMD_ERASE (0x80) // Requires a 2nd part for Sector or Whole Device
#define F_CMD_GETID (0x90)
#define F_CMD_PROG (0xA0)

// ====================================================================
// Data Types/Structures
// ====================================================================

// ====================================================================
// Data Section
// ====================================================================

static bool _initialized;

#define FDMFG_AMD "AMD"
#define FDMFG_MacChp "MicroChip"
#define FDMFG_Micnx "Micronix"

static md_info_t md_AMD_Am29F040B = {
    .mfgid = 0x01,
    .devid = 0xA4,
    .sectcnt = 8,
    .abm = 18,
    .mfgs = FDMFG_AMD,
    .devs = "Am29F040"
};

static md_info_t md_MC_SST39SF010 = {
    .mfgid = 0xBF,
    .devid = 0xB5,
    .sectcnt = 32,
    .abm = 16,
    .mfgs = FDMFG_MacChp,
    .devs = "SST39SF010A"
};

static md_info_t md_MC_SST39SF020 = {
    .mfgid = 0xBF,
    .devid = 0xB6,
    .sectcnt = 64,
    .abm = 17,
    .mfgs = FDMFG_MacChp,
    .devs = "SST39SF020A"
};

static md_info_t md_MC_SST39SF040 = {
    .mfgid = 0xBF,
    .devid = 0xB7,
    .sectcnt = 128,
    .abm = 18,
    .mfgs = FDMFG_MacChp,
    .devs = "SST39SF040"
};

static md_info_t md_MX_MX29F040 = {
    .mfgid = 0xC2,
    .devid = 0xA4,
    .sectcnt = 8,
    .abm = 18,
    .mfgs = FDMFG_Micnx,
    .devs = "MX29F040"
};

/** @brief The value of a byte that is considered empty */
#define MT_BYTE_VAL 0xFF

/** @brief Image for one sector (largest) of the programmable device */
static uint8_t _imgbuf[64*ONE_K];

/** @brief The size in bytes of the current device sector */
static uint32_t _sector_size;

static const md_info_t* mfgdev[] = {
    & md_AMD_Am29F040B,
    & md_MC_SST39SF010,
    & md_MC_SST39SF020,
    & md_MC_SST39SF040,
    & md_MX_MX29F040,
    NULL
};

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

static void _clr_device_buf() {
    memset(_imgbuf, MT_BYTE_VAL, _sector_size);
}

static bool _cmd_start(uint8_t cmd) {
    ERRORNO = 0;
    pdo_data_set_at(0x55555, 0xAA);
    if (ERRORNO < 0) {
        return (false);
    }
    pdo_data_set_at(0x2AAAA, 0x55);
    if (ERRORNO < 0) {
        return (false);
    }
    pdo_data_set_at(0x55555, cmd);
    if (ERRORNO < 0) {
        return (false);
    }
    return (true);
}

static void _cmd_end() {
    pdo_data_set_at(0, 0xF0);
}


// ====================================================================
// Public Methods
// ====================================================================

const md_info_t* pd_info() {
    _cmd_end(); // Just in case the device was left in a command state.
    if (!_cmd_start(F_CMD_GETID)) {
        return (NULL);
    }
    // If the power was on for the `_cmd_start` call, we'll assume that
    // it remains on. So we won't check after each operation.
    uint8_t mfgid = pdo_data_get_from(0);
    uint8_t devid = pdo_data_get_from(1);
    _cmd_end();
    //
    // Lookup the info
    const md_info_t* info = NULL;
    const md_info_t** indx = (const md_info_t**)mfgdev;
    while (*indx) {
        const md_info_t* ci = *indx;
        if (ci->mfgid == mfgid && ci->devid == devid) {
            info = ci;
            break;
        }
        indx++;
    }
    return (info);
}

bool pd_is_empty() {
    // First, get the device info to make sure we know what it is.
    const md_info_t* info = pd_info();
    if (!info) {
        return (false);
    }
    // Get the size (one more than the max address)
    uint32_t size = pd_size(info);
    // Scan the device looking for a non-empty byte
    for (uint32_t i = 0; i < size; i++) {
        uint8_t v = pdo_data_get_from(i);
        if (v != MT_BYTE_VAL) {
            return (false);
        }
    }
    return (true);
}

bool pd_is_sect_empty(uint8_t sect) {
    // First, get the device info to make sure we know what it is.
    const md_info_t* info = pd_info();
    if (!info) {
        return (false);
    }
    // Get the sector size and starting address.
    uint32_t sectsize = pd_sectsize(info);
    uint32_t saddr = pd_sectstart(info, sect);
    if (saddr == PD_INVALID_ADDR) {
        return false;
    }
    uint32_t end = saddr + sectsize;
    for (uint32_t i = saddr; i < end; i++) {
        uint8_t v = pdo_data_get_from(i);
        if (v != MT_BYTE_VAL) {
            return (false);
        }
    }
    return (true);
}

uint32_t pd_sectstart(const md_info_t* info, uint8_t sect) {
    if (sect >= info->sectcnt) {
        return (PD_INVALID_ADDR);
    }
    uint32_t sectsize = pd_sectsize(info);
    uint32_t saddr = sect * sectsize;
    return (saddr);
}

// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================

void pd_minit() {
    if (_initialized) {
        board_panic("!!! pd_module_init: Called more than once !!!");
    }
    _clr_device_buf();
    pdo_minit();
}
