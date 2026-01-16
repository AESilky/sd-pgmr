/**
 * Programmable Device operations.
 *
 * Maintains the image to be programmed and provides operations to load/modify/save it and
 * perform operations on the device to be programmed.
 *
 * A 1K buffer is used, so an SDCard must be available to read the image to be programmed
 * from. For loading from a host, the SDCard is used to contain a 'temp' file that holds
 * the image while operations are performed on the device.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include "prog_device.h"
#include "pdops.h"

#include "board.h"
#include "display.h"
#include "dskops.h"
#include "ihex.h"
#include "msgpost.h"
#include "shell.h"
#include "include/util.h"

#include "pico/types.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>  // For memset()


#define PD_TEMP_BIN_FILE_NAME "_tmp.bin"

#define F_CMD_ERASE1 (0x80) // Requires a 2nd part for Sector or Whole Device
#define F_CMD_ERASEPART (0x10)  // Requires 1st erase cmd, then this as 2nd cmd.
#define F_CMD_ERASEPARTADDR (0x55555) // Addr to write erase whole part to.

#define F_CMD_GETID (0x90)
#define F_CMD_PROG (0xA0)

// ====================================================================
// Data Types/Structures
// ====================================================================

// ====================================================================
// Data Section
// ====================================================================

static bool _initialized;

static volatile pd_op_status_t _method_status;

#define FDMFGID_AMD 0x01
#define FDMFG_AMD "AMD"
#define FDMFGID_MicroChp 0xBF
#define FDMFG_MicroChp "MicroChip"
#define FDMFGID_Micnx 0xC2
#define FDMFG_Micnx "Micronix"

static md_info_t md_AMD_Am29F040B = {
    .mfgid = FDMFGID_AMD,
    .devid = 0xA4,
    .sectcnt = 8,
    .abm = 18,
    .mfgs = FDMFG_AMD,
    .devs = "Am29F040"
};

static md_info_t md_MC_SST39SF010 = {
    .mfgid = FDMFGID_MicroChp,
    .devid = 0xB5,
    .sectcnt = 32,
    .abm = 16,
    .mfgs = FDMFG_MicroChp,
    .devs = "SST39SF010A"
};

static md_info_t md_MC_SST39SF020 = {
    .mfgid = FDMFGID_MicroChp,
    .devid = 0xB6,
    .sectcnt = 64,
    .abm = 17,
    .mfgs = FDMFG_MicroChp,
    .devs = "SST39SF020A"
};

static md_info_t md_MC_SST39SF040 = {
    .mfgid = FDMFGID_MicroChp,
    .devid = 0xB7,
    .sectcnt = 128,
    .abm = 18,
    .mfgs = FDMFG_MicroChp,
    .devs = "SST39SF040"
};

static md_info_t md_MX_MX29F040 = {
    .mfgid = FDMFGID_Micnx,
    .devid = 0xA4,
    .sectcnt = 8,
    .abm = 18,
    .mfgs = FDMFG_Micnx,
    .devs = "MX29F040"
};

/** @brief The value of a byte that is considered empty */
#define MT_BYTE_VAL 0xFF

#define PROG_OP_STATUS_INV 0x80
#define PROG_OP_STATUS_TGL 0x40
/** @brief Bit7 and Bit6 are the programming operation status bits */
#define PROG_OP_STATUS_BITS (PROG_OP_STATUS_INV | PROG_OP_STATUS_TGL)

/** @brief MicroChip Sector Erase sector to address adjust */
#define PD_MicroChp_SECT_ER_ADJ 12
#define PD_MicroChp_SECT_ER_CMD 0x30

#define IMAGE_BUF_SIZE ONE_K
/** @brief Buffer to read/write files from for the programmable device */
static uint8_t _imgbuf[IMAGE_BUF_SIZE];

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

static pd_op_status_t _pgm_byte(uint32_t addr, uint8_t b);

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

static uint8_t _chk_wr_status(uint8_t expected) {
    uint8_t v = pdo_data_get();
    uint8_t sb = (v & PROG_OP_STATUS_BITS);
    uint8_t v2;
    uint8_t s2;
    do {
        v2 = pdo_data_get();
        s2 = (v2 & PROG_OP_STATUS_BITS);
        if ((sb & PROG_OP_STATUS_TGL) == (s2 & PROG_OP_STATUS_TGL)) {
            break;
        }
        sb = s2;
    }
    while (v2 != expected);

    return (v2);
}

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

static bool _cmd_2nd(uint32_t addr, uint8_t cmd) {
    ERRORNO = 0;
    pdo_data_set_at(0x55555, 0xAA);
    if (ERRORNO < 0) {
        return (false);
    }
    pdo_data_set_at(0x2AAAA, 0x55);
    if (ERRORNO < 0) {
        return (false);
    }
    pdo_data_set_at(addr, cmd);
    if (ERRORNO < 0) {
        return (false);
    }
    return (true);
}

static void _cmd_end() {
    pdo_data_set_at(0, 0xF0);
}

static pd_op_status_t _pgm_byte(uint32_t addr, uint8_t b) {
    // Write a byte to the address
    ERRORNO = 0;
    _cmd_end(); // Just in case the device was left in a command state.
    uint8_t v = pdo_data_get_from(addr);
    if (ERRORNO != 0) {
        _method_status = PD_NOT_READY;
        goto _finally;
    }
    _method_status = PD_OP_OK;
    if (v != b) {
        if (v != MT_BYTE_VAL) {
            _method_status = PD_NOT_ERASED;
            goto _finally;
        }
        if (!_cmd_start(F_CMD_PROG)) {
            _method_status = PD_NOT_READY;
            goto _finally;
        }
        // If the power was on for the `pdo_...` call, we'll assume that it remains on.
        pdo_data_set_at(addr, b);
        // Get the device status
        uint8_t v2 = _chk_wr_status(b);
        if (v2 != b) {
            _method_status = PD_VERIFY_FAILED;
            goto _finally;
        }
    }
_finally:
    _cmd_end(); // Make sure we leave the device in a READ state
    return (_method_status);
}

static bool _process_hex_to_bin(const char* hexfilename, const progstat_handler_fn progstatfn) {
    FF_FILE* ffp = NULL;
    FF_FILE* tfp = NULL;
    FF_Stat_t fstat;
    bool retval = false;
    // Set up for a file error. We'll clear it once we are through the initial file ops.
    ERRORNO = _method_status = PD_FILE_OP_ERR;
    if (ff_stat(hexfilename, &fstat) != 0) {
        // Couldn't stat the file, so return.
        goto _finally;
    }
    // See if we can truncate or create the temp bin file.
    if ((tfp = ff_fopen(PD_TEMP_BIN_FILE_NAME, "w")) == NULL) {
        // Couldn't create/truncate file.
        goto _finally;
    }
    if ((ffp = ff_fopen(hexfilename, "r")) == NULL) {
        // Couldn't open file.
        goto _finally;
    }
    ihp_stat_t ihstat = ih_to_bin(ffp, tfp, progstatfn);
    retval = (ihstat == IHP_OK ? true : false);
_finally:
    if (tfp) {
        ff_fclose(tfp);
        tfp = NULL;
    }
    if (ffp) {
        ff_fclose(ffp);
        ffp = NULL;
    }
    return (retval);
}

// ====================================================================
// Public Methods
// ====================================================================

pd_op_status_t pd_erase_device(const md_info_t* info) {
    if (info->mfgid != FDMFGID_MicroChp) {
        _method_status = PD_DEV_NOSUP; // Currently, only support MicroChip
        return (_method_status);
    }
    _cmd_end(); // Just in case the device was left in a command state.
    if (!_cmd_start(F_CMD_ERASE1)) {
        _method_status = PD_NOT_READY;
        return (_method_status);
    }
    if (!_cmd_2nd(F_CMD_ERASEPARTADDR, F_CMD_ERASEPART)) {
        _method_status = PD_NOT_ERASED;
        return (_method_status);
    }
    pdo_addr_set(0);
    uint8_t sv = _chk_wr_status(MT_BYTE_VAL);
    _method_status = (sv == MT_BYTE_VAL ? PD_OP_OK : PD_ERASE_FAIL);
    return (_method_status);
}

pd_op_status_t pd_erase_sect(const md_info_t* info, uint8_t sect) {
    if (info->mfgid != FDMFGID_MicroChp) {
        _method_status = PD_DEV_NOSUP; // Currently, only support MicroChip
        return (_method_status);
    }
    if (sect >= info->sectcnt) {
        _method_status = PD_ADDR_INVALID;
        return (_method_status);
    }
    _cmd_end(); // Just in case the device was left in a command state.
    uint32_t seaddr = ((uint32_t)sect << PD_MicroChp_SECT_ER_ADJ);
    if (!_cmd_start(F_CMD_ERASE1)) {
        _method_status = PD_NOT_READY;
        return (_method_status);
    }
    if (!_cmd_2nd(seaddr, PD_MicroChp_SECT_ER_CMD)) {
        _method_status = PD_NOT_ERASED;
        return (_method_status);
    }
    uint8_t sv = _chk_wr_status(MT_BYTE_VAL);
    _method_status = (sv == MT_BYTE_VAL ? PD_OP_OK : PD_ERASE_FAIL);
    return (_method_status);
}

const md_info_t* pd_info() {
    _cmd_end(); // Just in case the device was left in a command state.
    if (!_cmd_start(F_CMD_GETID)) {
        _method_status = PD_NOT_READY;
        return (NULL);
    }
    // If the power was on for the `_cmd_start` call, we'll assume that
    // it remains on. So we won't check after each operation.
    uint8_t mfgid = pdo_data_get_from(0);
    uint8_t devid = pdo_data_get_from(1);
    _cmd_end();
    // If both are 0xFF it is probably due to no device installed.
    if (mfgid == 0xFF && devid == 0xFF) {
        _method_status = PD_NO_DEVICE;
        return (NULL);
    }
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
    _method_status = (info ? PD_OP_OK : PD_NOT_IDENTIFIED);
    return (info);
}

bool pd_is_empty(const progstat_handler_fn progstatfn) {
    // First, get the device info to make sure we know what it is.
    const md_info_t* info = pd_info();
    if (!info) {
        return (false);
    }
    // Get the size (one more than the max address)
    uint32_t size = pd_size(info);
    // Scan the device looking for a non-empty byte
    uint32_t addr = 0;
    while (addr < size) {
        for (uint32_t i = 0; i < ONE_K && addr < size; i++) {
            if (progstatfn) {
                if (progstatfn(PDS_PROC_BYTE, addr, 0, 0) != 0) {
                    _method_status = PD_OP_CANCELLED;
                    return (false);
                }
            }
            uint8_t v = pdo_data_get_from(addr++);
            if (v != MT_BYTE_VAL) {
                if (progstatfn) {
                    // Don't worry about 'cancel' - as we are done.
                    progstatfn(PDS_NOT_ERASED, addr - 1, v, 0);
                }
                _method_status = PD_NOT_ERASED;
                return (false);
            }
        }
    }
    _method_status = PD_OP_OK;
    return (true);
}

bool pd_is_sect_empty(uint8_t sect, const progstat_handler_fn progstatfn) {
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
            _method_status = PD_NOT_ERASED;
            return (false);
        }
    }
    _method_status = PD_OP_OK;
    return (true);
}

pd_op_status_t pd_method_status() {
    return _method_status;
}

pd_op_status_t pd_prog_file(const md_info_t* info, const char* filename, const progstat_handler_fn progstatfn) {
    FF_FILE* fp = NULL;
    FF_Stat_t fstat;
    // Set up for a file error. We'll clear it once we are through the initial file ops.
    ERRORNO = _method_status = PD_FILE_OP_ERR;
    if (ff_stat(filename, &fstat) != 0) {
        return (_method_status);
    }
    fp = ff_fopen(filename, "r");
    if (!fp) {
        ERRORNO = _method_status = PD_FILE_OP_ERR;
        return (_method_status);
    }
    // See if it is IntelHEX
    bool ishexfile = ih_is_hexfile(fp);
    if (ishexfile) {
        // Yes, it's a Hex file, process it to binary and then proceed.
        if (progstatfn) {
            if (progstatfn(PDS_FILETYPE_HEX, 0, 0, 0) !=0) {
                _method_status = PD_OP_CANCELLED;
                goto _finally;
            }
        }
        ff_fclose(fp);
        fp = NULL;
        if (!_process_hex_to_bin(filename, progstatfn)) {
            goto _finally;
        }
        // Process the _tmp.bin file.
        filename = PD_TEMP_BIN_FILE_NAME;
        if (ff_stat(filename, &fstat) != 0) {
            goto _finally;
        }
        fp = ff_fopen(filename, "r");
        if (!fp) {
            ERRORNO = _method_status = PD_FILE_OP_ERR;
            return (_method_status);
        }
    }
    else {
        if (progstatfn) {
            if (progstatfn(PDS_FILETYPE_BIN, 0, 0, 0) != 0) {
                _method_status = PD_OP_CANCELLED;
                goto _finally;
            }
        }
    }
    // Get the info about the device
    uint32_t pdsize = pd_size(info);
    if (fstat.st_size > pdsize) {
        ERRORNO = _method_status = PD_DEVICE_SIZE;
        goto _finally;
    }
    // The file is open for reading and it will fit on the device.
    ERRORNO = _method_status = PD_PROG_FAILED;
    uint32_t addr = 0;
    uint32_t addrmax = pd_addrmax(info);
    while (addr <= addrmax && addr < fstat.st_size) {
        // Read 'IMAGE_BUF_SIZE' (1K) from the file
        size_t br = ff_fread(&_imgbuf, sizeof(uint8_t), IMAGE_BUF_SIZE, fp);
        if (br == 0 && addr != (fstat.st_size - 1)) {
            // Didn't read as many bytes as the stat indicates
            ERRORNO = _method_status = PD_FILE_OP_ERR;
            goto _finally;
        }
        for (int i = 0; i < br; i++) {
            // Get a byte from the buffer and Write it
            uint8_t b = _imgbuf[i];
            if (progstatfn) {
                if (progstatfn(PDS_PROC_BYTE, addr, b, 0) != 0) {
                    _method_status = PD_OP_CANCELLED;
                    goto _finally;
                }
            }
            if (_pgm_byte(addr, b) != PD_OP_OK) {
                // _method_status is set by `_pgm_byte`
                goto _finally;
            }
            addr++;
            if (addr > addrmax || addr == fstat.st_size) {
                break;
            }
        }
    }
    if (addr == fstat.st_size) {
        ERRORNO = _method_status = PD_OP_OK;
    }
_finally:
    // Close the file to free resources
    if (fp) {
        ff_fclose(fp);
    }
    return (_method_status);
}

pd_op_status_t pd_read_to_fb(const md_info_t* info, const char* filename, const progstat_handler_fn progstatfn) {
    FF_FILE* fp = NULL;
    // Set up for a file error. We'll clear it once we are through the initial file ops.
    ERRORNO = _method_status = PD_FILE_OP_ERR;
    fp = ff_fopen(filename, "w"); // Create New or Truncate and open for Write
    if (!fp) {
        ERRORNO = _method_status = PD_FILE_OP_ERR;
        return (_method_status);
    }
    // The file is open for writing.
    ERRORNO = _method_status = PD_READ_FAILED;
    uint32_t addr = 0;
    uint32_t addrmax = pd_addrmax(info);
    _cmd_end(); // Just in case the device was left in a command state.
    while (addr <= addrmax) {
        memset(_imgbuf, MT_BYTE_VAL, IMAGE_BUF_SIZE); // Fill with the empty value.
        for (int i = 0; i < IMAGE_BUF_SIZE && addr <= addrmax; i++) {
            // Read it
            uint8_t v = pdo_data_get_from(addr);
            if (ERRORNO != 0) {
                goto _finally;
            }
            if (progstatfn) {
                if (progstatfn(PDS_PROC_BYTE, addr, v, 0) != 0) {
                    _method_status = PD_OP_CANCELLED;
                    goto _finally;
                }
            }
            *(_imgbuf + i) = v;
            addr++;
        }
        // Write 'IMAGE_BUF_SIZE' (1K) to the file
        size_t br = ff_fwrite(&_imgbuf, sizeof(uint8_t), IMAGE_BUF_SIZE, fp);
        if (br != IMAGE_BUF_SIZE) {
            ERRORNO = _method_status = PD_FILE_OP_ERR;
            goto _finally;
        }
    }
    if (addr == (addrmax+1)) {
        ERRORNO = _method_status = PD_OP_OK;
    }
_finally:
    _cmd_end();
    // Close the file to free resources
    if (fp) {
        ff_fclose(fp);
    }
    return (_method_status);
}

uint8_t pd_read_value(const md_info_t* info, uint32_t addr) {
    uint32_t maxaddr = pd_addrmax(info);
    if (addr > maxaddr) {
        _method_status = PD_ADDR_INVALID;
        return (0xFF);
    }
    uint8_t v = pdo_data_get_from(addr);
    _method_status = PD_OP_OK;
    return (v);
}

uint32_t pd_sectstart(const md_info_t* info, uint8_t sect) {
    if (sect >= info->sectcnt) {
        return (PD_INVALID_ADDR);
    }
    uint32_t sectsize = pd_sectsize(info);
    uint32_t saddr = sect * sectsize;
    _method_status = PD_OP_OK;
    return (saddr);
}

pd_op_status_t pd_verify_fb(const md_info_t* info, const char* filename, uint32_t* lastaddr, const progstat_handler_fn progstatfn) {
    FF_FILE* fp = NULL;
    FF_Stat_t fstat;
    // Set up for a file error. We'll clear it once we are through the initial file ops.
    ERRORNO = _method_status = PD_FILE_OP_ERR;
    if (ff_stat(filename, &fstat) != 0) {
        return (_method_status);
    }
    // Get the info about the device
    uint32_t pdsize = pd_size(info);
    if (fstat.st_size > pdsize) {
        ERRORNO = _method_status = PD_DEVICE_SIZE;
        return (_method_status);
    }
    fp = ff_fopen(filename, "r");
    if (!fp) {
        ERRORNO = _method_status = PD_FILE_OP_ERR;
        return (_method_status);
    }
    // The file is open for reading and it will fit on the device.
    ERRORNO = _method_status = PD_VERIFY_FAILED;
    uint32_t addr = 0;
    uint32_t addrmax = pd_addrmax(info);
    _cmd_end(); // Just in case the device was left in a command state.
    while (addr <= addrmax && addr < fstat.st_size) {
        // Read 'IMAGE_BUF_SIZE' (1K) from the file
        size_t br = ff_fread(&_imgbuf, sizeof(uint8_t), IMAGE_BUF_SIZE, fp);
        if (br == 0 && addr != (fstat.st_size - 1)) {
            // Didn't read as many bytes as the stat indicates
            ERRORNO = _method_status = PD_FILE_OP_ERR;
            goto _finally;
        }
        for (int i = 0; i < br; i++) {
            // Verify it
            uint8_t b = _imgbuf[i];
            uint8_t v = pdo_data_get_from(addr);
            if (progstatfn) {
                if (progstatfn(PDS_PROC_BYTE, addr, v, b) != 0) {
                    _method_status = PD_OP_CANCELLED;
                    goto _finally;
                }
            }
            if (ERRORNO != 0) {
                goto _finally;
            }
            if (v != b) {
                // Data wasn't the same
                goto _finally;
            }
            addr++;
            if (addr > addrmax || addr == fstat.st_size) {
                break;
            }
        }
    }
    if (addr == fstat.st_size) {
        ERRORNO = _method_status = PD_OP_OK;
    }
_finally:
    _cmd_end();
    *lastaddr = addr;
    // Close the file to free resources
    if (fp) {
        ff_fclose(fp);
    }
    return (_method_status);
}

pd_op_status_t pd_write_value(const md_info_t* info, uint32_t addr, uint8_t value) {
    uint32_t maxaddr = pd_addrmax(info);
    if (addr > maxaddr) {
        _method_status = PD_ADDR_INVALID;
        return (0xFF);
    }
    _pgm_byte(addr, value); // This sets `_method_status`
    return (_method_status);
}



// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================

void pd_modinit() {
    if (_initialized) {
        board_panic("!!! pd_module_init: Called more than once !!!");
    }
    _clr_device_buf();
    pdo_modinit();
    ih_modinit();
    _method_status = PD_OP_OK;
}
