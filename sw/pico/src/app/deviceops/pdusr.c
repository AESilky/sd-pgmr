/**
 * @brief Programmable Device User Operations
 * @file pdusr.c
 * @ingroup device
 *
 * Used by both the command shell and the Rotary+Switch and Display to keep them
 * consistent.
 *
 * Copyright 2025 AESilky
 * SPDX-License-Identifier: MIT License
 */

#include "pdusr.h"

#include "pdops.h"

#include "board.h"
#include "cmt.h"
#include "display.h"
#include "dskops.h"
#include "hwrt_t.h"
#include "menumgr.h"
#include "shell.h"
#include "include/util.h"

#include <string.h>


#define _TMP_BUF_LEN    80

// ====================================================================
// Data Section
// ====================================================================

static volatile bool _initialized;

static const display_info_t* _disp_info;
static int _disp_ll;
static dlg_ctx_t* _dialog;
static const char* _process;
static bool _shell_out;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================

static void _on_dlg_cancel(cmt_msg_t* msg);
static void _reactivate_menu(void* data);


// ====================================================================
// Local/Private Methods
// ====================================================================

static bool _chk_file_can_read(const char* filename) {
    bool retval = true;
    FF_FILE* file = ff_fopen(filename, "r");
    if (!file) {
        retval = false;
        if (_shell_out) {
            shell_printferr("Cannot open file '%s'\n", filename);
        }
        display_line(2, "Cannot", DISP_JUSTIFY_CENTER, false, false, NoPaint);
        display_line(3, "Open File", DISP_JUSTIFY_CENTER, false, false, Paint);
    }
    // We can close the file. The operation function will open it to use it.
    ff_fclose(file);
    return (retval);
}

static void _err_device_size(const md_info_t* info, const char* filename) {
    uint32_t pdsize = pd_size(info);
    FF_Stat_t fstat;
    int fsize = -1;
    if (ff_stat(filename, &fstat) != 0) {
        if (_shell_out) {
            shell_printferr("Cannot stat '%s'\n", filename);
        }
    }
    else {
        fsize = fstat.st_size;
    }
    if (_shell_out) {
        shell_printferr("File image size (%d) is larger than device (%d).\n", fsize, pdsize);
    }
    display_line(2, "Image too large", DISP_JUSTIFY_CENTER, false, false, Paint);
}

static const md_info_t* _get_device_info() {
    // Get the device info
    const md_info_t* info = pd_info();
    if (!info) {
        if (_shell_out) {
            shell_printferr("Device not identified.\n");
        }
        display_line(2, "DEVICE", DISP_JUSTIFY_CENTER, false, false, NoPaint);
        display_line(3, "UNKNOWN", DISP_JUSTIFY_CENTER, false, false, Paint);
    }
    return (info);
}

static void _op_leave() {
    // Try to turn the power off
    pdo_pwr_request_on(false);
    _shell_out = false;
    // Create a blank cancel dialog to allow the user to exit the display early.
    _dialog = dlg_confirm_notext(8000, _on_dlg_cancel, _on_dlg_cancel);
}

static void _op_enter(const char* procstr, bool shellout) {
    // Deactivate the menus on the display and set shell output for status
    _process = procstr;
    _shell_out = shellout;
    menu_deactivate();
    display_clear(true);
    display_line(0, _process, DISP_JUSTIFY_CENTER, true, false, Paint);
    if (_shell_out) {
        shell_puts(procstr);
    }
    attn_clear();   // Clear attention so we can watch for cancel
}

// ====================================================================
// Run-After/Delay/Sleep Methods
// ====================================================================

/**
 * @brief Reactivate the menu on the display. Called after delay after command ends.
 *
 * @param data Nothing important (can be pointer to anything needed)
 */
static void _reactivate_menu(void* data) {
    menu_display_current();
}


// ====================================================================
// Message Handler Methods
// ====================================================================

/**
 * @brief Handle cancel dialog.
 *
 * @param msg .
 */
static void _on_dlg_cancel(cmt_msg_t* msg) {
    dlg_dismiss(_dialog);
    _dialog = NULL;
    cmt_run_after_ms(80, _reactivate_menu, NULL);
}

/**
 * @brief Handle our Housekeeping tasks. This is triggered every ~16ms.
 *
 * Triggered at 62.5Hz, so 625 times is 10 seconds.
 *
 * @param msg Nothing important in the message.
 */
static void _handle_housekeeping(cmt_msg_t* msg) {
    static uint cnt = 0;

    cnt++;
}


// ====================================================================
// Callback Methods
// ====================================================================

/**
 * @brief Print and Display status. Check to see if the operation should be cancelled.
 *
 * @param stat
 * @param x
 * @param y
 * @param z
 * @return true Cancel the operation
 * @return false Don't cancel
 */
bool _on_progress(pd_status_type stat, uint32_t x, uint32_t y, uint32_t z) {
    // Provide some info to the user.
    char buf[_TMP_BUF_LEN];

    switch (stat) {
    case PDS_FILETYPE_BIN:
        if (_shell_out) {
            shell_puts("\n File is binary. Working:");
        }
        display_line(1, "(Bin File)", DISP_JUSTIFY_CENTER, false, false, Paint);
        break;
    case PDS_FILETYPE_HEX:
        if (_shell_out) {
            shell_puts("\n File is HEX, converting.\n");
        }
        display_line(1, "Converting", DISP_JUSTIFY_LEFT, false, false, NoPaint);
        display_line(2, "Nex File", DISP_JUSTIFY_LEFT, false, false, Paint);
        break;
    case PDS_PROC_HEX_REC:
        // x is the record number
        if (x % 100 == 0) {
            const char* lead = (x > 100 ? "\e[6D" : "  Processing record: ");
            if (_shell_out) {
                shell_printf("%s%6u", lead, x);
            }
            sprintf(buf, "REC:%6u", x);
            display_line(3, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
        }
        break;
    case PDS_PROC_HEX_CMPT:
        // x is OK/ERROR indicator, y is record with error (if one)
        if (x) {
            if (_shell_out) {
                shell_printf("\nHex file error. Record: %u\n", y);
            }
            sprintf(buf, " %u", x);
            display_line(3, "ERROR AT:", DISP_JUSTIFY_LEFT, false, false, NoPaint);
            display_line(4, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
        }
        else {
            if (_shell_out) {
                shell_puts("\n Hex file converted to binary.\n Continuing:");
            }
            display_line(3, "Converted", DISP_JUSTIFY_LEFT, false, false, NoPaint);
            display_line(4, "", DISP_JUSTIFY_LEFT, false, false, Paint);
        }
        break;
    case PDS_PROC_BYTE:
        // x is the byte number
        // y is the byte value
        if (x == 90) {
            display_clear(false);
            display_line(0, _process, DISP_JUSTIFY_CENTER, true, false, NoPaint);
            display_string(2, 0, "Loc:", false, false, NoPaint);
        }
        if (x % ONE_K == 0) {
            if (_shell_out) {
                shell_putc('.');
            }
            sprintf(buf, "%05X", x);
            display_string(2, 5, buf, false, false, Paint);
        }
        break;
    case PDS_NOT_ERASED:
        // x is the byte number
        // y is the byte value
        if (_shell_out) {
            shell_printf("\nDevice not blank at location: %05X Value: %02X\n", x, y);
        }
        sprintf(buf, "A:%05X D:%02X", x, y);
        display_line(3, "NOT EMPTY", DISP_JUSTIFY_LEFT, false, false, NoPaint);
        display_line(5, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
        break;
    case PDS_DATA_MISMATCH:
        // x is location, y is 'expected', z is 'read'
        if (_shell_out) {
            shell_printf("\nData mismatch at: %05X Expected: %02X Was: %02X\n", x, y, z);
        }
        sprintf(buf, "A:%05X E:%02X R:%02X", x, y);
        display_line(3, "MISMATCH", DISP_JUSTIFY_LEFT, false, false, NoPaint);
        display_line(4, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
        break;
    case PDS_COMPLETED:
        if (_shell_out) {
            shell_puts("\nComplete.\n");
        }
        display_clear(false);
        display_line(1, "Complete", DISP_JUSTIFY_CENTER, false, false, NoPaint);
        break;
    }
    bool attn = attn_is_set();
    if (stat != PDS_COMPLETED && attn) {
        // Cancel
        if (_shell_out) {
            shell_puts("Cancelled\n");
        }
        display_line(4, "CANCELLED", DISP_JUSTIFY_CENTER, false, false, Paint);
    }
    return (attn);
}

// ====================================================================
// Local/Private Methods
// ====================================================================


// ====================================================================
// Public Methods
// ====================================================================

bool pdusr_erase_all(bool shellout) {
    bool retval = false;
    char buf[20];
    _op_enter("Erasing", shellout);
    // Try to turn the power on
    ERRORNO = 0;
    if (!pdusr_pwr_request_on(true, shellout)) {
        goto _finally;
    }
    // Get the device info
    const md_info_t* info = _get_device_info();
    if (!info) {
        goto _finally;
    }
    if (_shell_out) {
        shell_puts("\n erasing device...");
    }
    display_line(2, "erasing...", DISP_JUSTIFY_CENTER, false, false, NoPaint);
    pd_op_status_t stat = pd_erase_device(info);
    if (stat != PD_OP_OK) {
        if (_shell_out) {
            shell_printf("\nError erasing device: (%d)\n", stat);
        }
        sprintf(buf, "ERROR (%u)", stat);
        display_line(2, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
    }
    else {
        if (_shell_out) {
            shell_puts("\nDevice erased.\n");
        }
        display_line(2, "ERASED", DISP_JUSTIFY_CENTER, false, false, Paint);
    }
_finally:
    _op_leave();
    return (retval);
}

bool pdusr_erase_sect(const md_info_t* info, int sect, bool shellout) {
    bool retval = false;
    char buf[_TMP_BUF_LEN];
    _op_enter("Erase Sect", shellout);
    if (_shell_out) {
        shell_printf("\nerasing sector %hu...", sect);
    }
    sprintf(buf, "Sector: %u", sect);
    display_line(2, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
    pd_op_status_t stat = pd_erase_sect(info, sect);
    if (stat != PD_OP_OK) {
        if (_shell_out) {
            shell_printf("\nError erasing sector %hu: (%d)\n", sect, stat);
        }
        sprintf(buf, "Error: %u", stat);
        display_line(3, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
    }
    else {
        retval = true;
        if (_shell_out) {
            shell_printf("\nSector %hu erased.\n", sect);
        }
        display_line(3, "Erased", DISP_JUSTIFY_LEFT, false, false, Paint);
    }
    _op_leave();
    return (retval);
}

const md_info_t* pdusr_info(bool errs_only, bool shellout) {
    if (!errs_only) {
        _op_enter("Info", shellout);
    }
    char buf[_TMP_BUF_LEN];
    _shell_out = shellout;
    // Try to turn the power on
    ERRORNO = 0;
    if (!pdusr_pwr_request_on(true, shellout)) {
        if (_shell_out) {
            shell_printferr("Unable to power on the device.\n"); // ZZZ
        }
        goto _finally;
    }
    const md_info_t* info = pd_info();
    pdo_pwr_request_on(false);
    if (!info) {
        if (_shell_out) {
            shell_printferr("Device not identified.\n"); // ZZZ
        }
        goto _finally;
    }
    if (!errs_only) {
        uint32_t size = pd_size(info);
        uint16_t ksize = size / ONE_K;
        uint32_t sectsize = pd_sectsize(info);
        uint16_t ksectsize = sectsize / ONE_K;

        _process = "Device Info";
        display_clear(true);
        display_line(0, _process, DISP_JUSTIFY_CENTER, true, false, NoPaint);
        strcpynt(buf, info->mfgs, _disp_ll);
        display_string(1, 0, buf, false, false, NoPaint);
        strcpynt(buf, info->devs, _disp_ll);
        display_string(2, 0, buf, false, false, NoPaint);
        snprintf(buf, _disp_ll + 1, "%u Bytes", size);
        display_string(3, 0, buf, false, false, NoPaint);
        snprintf(buf, _disp_ll + 1, "%huK", ksize);
        display_string(4, 0, buf, false, false, NoPaint);
        snprintf(buf, _disp_ll + 1, "Sec %hux%huK", (uint16_t)info->sectcnt, ksectsize);
        display_string(5, 0, buf, false, false, Paint);
        if (_shell_out) {
            shell_printf("\n Device - MFG:%s DEV:%s Size: %huK Sectors:%hu x %huK\n", info->mfgs, info->devs, ksize, (uint16_t)info->sectcnt, ksectsize);
        }
    }
_finally:
    if (!errs_only) {
        _op_leave();
    }
    return (info);
}

bool pdusr_is_empty(bool shellout) {
    bool retval = false;
    _op_enter("Check Blank", shellout);
    // Try to turn the power on
    if (!pdusr_pwr_request_on(true, shellout)) {
        goto _finally;
    }
    const md_info_t* info = _get_device_info();
    if (!info) {
        goto _finally;
    }
    if (_shell_out) {
        shell_printf("\n checking device");
    }
    display_line(2, "checking...", DISP_JUSTIFY_CENTER, false, false, NoPaint);
    bool ismt = pd_is_empty(_on_progress);
    if (_shell_out) {
        const char* mods = (ismt ? "" : "not ");
        shell_printf("\nDevice is %sblank\n", mods);
    }
    if (!ismt) {
        display_line(2, "Not", DISP_JUSTIFY_CENTER, false, false, NoPaint);
    }
    display_line(3, "Blank", DISP_JUSTIFY_CENTER, false, false, Paint);
_finally:
    _op_leave();

    return (retval);
}

bool pdusr_is_sect_empty(int sect, bool shellout) {
    bool retval = false;
    _op_enter("Verify Sect", shellout);
    // Try to turn the power on
    if (!pdusr_pwr_request_on(true, true)) {
        goto _finally;
    }
    if (shellout) {
        shell_printf("checking device...");
    }
    bool mt = pd_is_sect_empty(sect, _on_progress);
    const char* mods = (mt ? "" : "not ");
    shell_printf("\nDevice sector %hu is blank\n", sect, mods);
_finally:
    // Try to turn the power off
    pdo_pwr_request_on(false);

    return (retval);
}

pd_op_status_t pdusr_prog(const char* filename, bool shellout) {
    pd_op_status_t retval = PD_PROG_FAILED;
    _op_enter("Program", shellout);
    // Check that the file can be opened to read
    if (!_chk_file_can_read(filename)) {
        goto _finally;
    }
    // Try to turn the power on
    if (!pdusr_pwr_request_on(true, shellout)) {
        goto _finally;
    }
    const md_info_t* info = _get_device_info();
    if (!info) {
        goto _finally;
    }
    pd_op_status_t pdos = pd_prog_file(info, filename, _on_progress);
    if (pdos == PD_OP_OK) {
        if (_shell_out) {
            shell_puts("\nProgrammed\n");
        }
        display_line(4, "PROGRAMMED", DISP_JUSTIFY_CENTER, false, false, Paint);
    }
    else if (pdos != PD_OP_CANCELLED) {
        if (pdos == PD_DEVICE_SIZE) {
            _err_device_size(info, filename);
        }
        else {
            if (_shell_out) {
                shell_printferr("\nCould not program device (%d)\n", pdos);
            }
            display_line(4, "NOT", DISP_JUSTIFY_CENTER, false, false, NoPaint);
            display_line(5, "PROGRAMMED", DISP_JUSTIFY_CENTER, false, false, Paint);
        }
    }
_finally:
    _op_leave();
    return (retval);
}

pd_op_status_t pdusr_read(const char* filename, bool shellout) {
    pd_op_status_t retval = PD_READ_FAILED;
    char buf[_TMP_BUF_LEN];
    _op_enter("Read", shellout);
    // Try to turn the power on
    if (!pdusr_pwr_request_on(true, shellout)) {
        goto _finally;
    }
    const md_info_t* info = _get_device_info();
    if (!info) {
        goto _finally;
    }
    if (_shell_out) {
        shell_puts("\nReading");
    }
    display_line(2, "Reading", DISP_JUSTIFY_CENTER, false, false, NoPaint);
    pd_op_status_t pdos = pd_read_to_fb(info, filename, _on_progress);
    if (_shell_out) {
        shell_putc('\n');
    }
    retval = pdos;
    if (pdos == PD_OP_OK) {
        if (_shell_out) {
            shell_puts("\nRead to file\n");
        }
        display_line(4, "READ DONE", DISP_JUSTIFY_CENTER, false, false, Paint);
    }
    else {
        if (pdos == PD_DEVICE_SIZE) {
            _err_device_size(info, filename);
        }
        else if (pdos == PD_READ_FAILED) {
            if (_shell_out) {
                shell_puts("Device could not be read or file written.\n");
            }
            display_line(2, "Not read", DISP_JUSTIFY_LEFT, false, false, NoPaint);
            display_line(3, "to file", DISP_JUSTIFY_LEFT, false, false, Paint);
        }
        else if (pdos != PD_OP_CANCELLED) {
            shell_printferr("Error reading to file: %u\n", pdos);
            sprintf(buf, "Error: %u", pdos);
            display_line(2, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
        }
    }
_finally:
    _op_leave();
    return (retval);
}

pd_op_status_t pdusr_verify(const char* filename, bool shellout) {
    pd_op_status_t retval = PD_VERIFY_FAILED;
    char buf[_TMP_BUF_LEN];
    _op_enter("Verify", shellout);
    // Check that the file can be opened to read
    if (!_chk_file_can_read(filename)) {
        goto _finally;
    }
    // Try to turn the power on
    if (!pdusr_pwr_request_on(true, shellout)) {
        goto _finally;
    }
    const md_info_t* info = _get_device_info();
    if (!info) {
        goto _finally;
    }
    uint32_t lastaddr;
    if (_shell_out) {
        shell_puts("Verifying");
    }
    display_line(2, "Verifying", DISP_JUSTIFY_CENTER, false, false, NoPaint);
    pd_op_status_t pdos = pd_verify_fb(info, filename, &lastaddr, _on_progress);
    if (_shell_out) {
        shell_putc('\n');
    }
    retval = pdos;
    if (pdos == PD_OP_OK) {
        if (_shell_out) {
            shell_puts("\nVerified\n");
        }
        display_line(4, "VERIFIED", DISP_JUSTIFY_CENTER, false, false, Paint);
    }
    else {
        if (pdos == PD_DEVICE_SIZE) {
            _err_device_size(info, filename);
        }
        else if (pdos == PD_VERIFY_FAILED) {
            shell_printferr("Device did not verify. Mismatch at %05X\n", lastaddr);
            display_line(2, "Mismatch", DISP_JUSTIFY_LEFT, false, false, NoPaint);
            sprintf(buf, "Location: %05X", lastaddr);
            display_line(3, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
        }
        else if (pdos != PD_OP_CANCELLED) {
            shell_printferr("Error verifying device (%d)\n", pdos);
            sprintf(buf, "Error: %u", pdos);
            display_line(2, buf, DISP_JUSTIFY_LEFT, false, false, Paint);
        }
    }
_finally:
    _op_leave();
    return (retval);
}

bool pdusr_pwr_request_on(bool on, bool shellout) {
    // Try to turn the power on
    ERRORNO = 0;
    if (!pdo_pwr_request_on(on)) {
        if (on) { // Only output if trying to turn on
            if (shellout) {
                shell_printferr("Unable to power on the device.\n");
            }
            display_line(2, "Cannot Power", DISP_JUSTIFY_CENTER, false, false, Paint);
            return (false);
        }
    }
    return (true);
}



// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================


void pdusr_modinit() {
    if (_initialized) {
        board_panic("!!! pdusr_modinit: Called more than once !!!");
    }
    _initialized = true;

    _disp_info = display_info();
    _disp_ll = _disp_info->cols;
}

