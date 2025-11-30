/**
 * Commends: Debug.
 *
 * Shell commands for the Programmable Device.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */

#include "cmds.h"

#include "cmt.h"
#include "include/util.h"

#include "shell/include/shell.h"
#include "shell/cmd/cmd_t.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "deviceops/include/pdops.h"

#define DDRDWR_REPEAT_MS 10

typedef enum RPTOP_ {
    RPT_NONE,
    RPT_ADDR_SET,
    RPT_WR_DATA,
    RPT_RD_DATA
} rptop_t;

static uint32_t _addr;
static uint8_t _data;
static uint8_t _sect;  // Sector (set from address conversion or directly)
static bool _repeat;
static rptop_t _rptop;
static bool _rptdlyip; // True if a repeat delay has been scheduled and not received.


const cmd_handler_entry_t cmds_addrtosect_entry;
const cmd_handler_entry_t cmds_devaddr_entry;
const cmd_handler_entry_t cmds_devaddr_n_entry;
const cmd_handler_entry_t cmds_devdump_entry;
const cmd_handler_entry_t cmds_devpwr_entry;
const cmd_handler_entry_t cmds_devrd_entry;
const cmd_handler_entry_t cmds_devrd_n_entry;
const cmd_handler_entry_t cmds_devwr_entry;
const cmd_handler_entry_t cmds_devwr_n_entry;


/**
 * @brief Get an unsigned value under a limit from a string or "." to keep the current value.
 *
 * This loads `val` with the value from `str` if it is a valid value from 0 to less than limit.
 * It leaves the `val` unchanged if the `str` is ".".
 *
 * If the str isn't valid or the value is greater than FFFF a message is printed to the shell.
 *
 * @param val Pointer to an unsigned word (16-bit)
 * @param str String to parse for the value
 * @param limit The maximum value allowed
 * @param hex True to process a hex value. False for decimal.
 * @param err_type String with a word(s) describing the type of expected value (used in the error message)
 * @return true Good conversion
 * @return false Invalid hex string or value larger than FFFF
 */
static bool _get_val(uint32_t* val, const char* str, uint32_t limit, bool hex, const char* err_type) {
    bool valid;
    uint32_t v;
    if (strcmp(".", str) != 0) {
        if (hex) {
            v = uint_from_hexstr(str, &valid);
        }
        else {
            v = uint_from_str(str, &valid);
        }
        if (!valid) {
            char* type = (hex ? "HEX" : "decimal");
            shell_printferr("Value error - '%s' is not valid %s.\n", str, type);
            return (false);
        }
        if (v > limit) {
            shell_printferr("Value error - '%s' is not a valid %s.\n", str, err_type);
            return (false);
        }
        *val = v;
    }
    return (true);
}

static bool _get_addr(char* addrstr) {
    uint32_t addr = _addr;
    if (!_get_val(&addr, addrstr, 0x7FFFF, true, "hex address")) {
        return (false);
    }
    if (addr != _addr) {
        // The address is different, set the address on the device.
        _addr = addr;
        pdo_addr_set(_addr);
        if (ERRORNO) {
            return false;
        }
    }
    return (true);
}

static void _repeat_handler(cmt_msg_t *msg) {
    _rptdlyip = false;  // Delay completed
    // Do the operation
    switch(_rptop) {
        case RPT_ADDR_SET:
            pdo_addr_set(_addr);
            break;
        case RPT_WR_DATA:
            pdo_data_set(_data);
            break;
        case RPT_RD_DATA:
            uint8_t data = pdo_data_get();
            _data = data; // Just for debugging
            break;
        default:
            _repeat = false;
            break;
    }
    if (_repeat) {
        // Repeat hasn't been cancelled... Schedule another.
        cmt_msg_t msg1;
        cmt_exec_init(&msg1, _repeat_handler);
        schedule_msg_in_ms(DDRDWR_REPEAT_MS, &msg1);
        _rptdlyip = true;
    }
}

static int _exec_atos(int argc, char** argv, const char* unparsed) {
    if (argc != 2) {
        // We take exactly 1 argument.
        cmd_help_display(&cmds_addrtosect_entry, HELP_DISP_USAGE);
        return (-1);
    }
    uint32_t addr = _addr;
    // Arg is address (HEX or '.').
    bool valid = _get_val(&addr, argv[1], 0x7FFFF, true, "hex address");
    if (!valid) {
        return (-1);
    }
    _addr = addr;
    _sect = addr / PD_SECTSIZE;
    shell_printf("Addr: %05X  Sector: %hu\n", addr, _sect);
    return (0);
}

static int _exec_addr(int argc, char** argv, const char* unparsed) {
    if (argc > 2) {
        // We only take 0 or 1 argument.
        cmd_help_display(&cmds_devaddr_entry, HELP_DISP_USAGE);
        return (-1);
    }
    // Try to turn the power on
    pdo_request_pwr_on(true);
    int retval = 0;
    _rptop = RPT_NONE;  // no repeat unless enabled below
    _repeat = false; // using this command with other than 'R' stops any repeat operation.
    if (argc > 1) {
        // The arg is the address to use (hex) or 'R' to do a repetitive read operation.
        if (tolower(*argv[1]) == 'r') {
            _rptop = RPT_ADDR_SET;
            _repeat = true;
        }
        else {
            // Get an address and set it.
            char* addrstr = argv[1];
            if (!_get_addr(addrstr)) {
                // _get_addr tells the user the address wasn't valid - just exit.
                retval = -1;
                goto _finally;
            }
        }
    }
    // Display the address
    shell_printf("%05X\n", _addr);
    // If they don't want a repeated address set, cancel any op-in-progress
    if (!_repeat) {
        if (_rptdlyip) {
            scheduled_msg_cancel2(MSG_EXEC, _repeat_handler);
        }
    }
    if (_repeat) {
        // They want a repeated address set, kick it off
        _repeat_handler(NULL);
    }

_finally:
    // Try to turn the power off
    pdo_request_pwr_on(false);

    return (retval);
}

static int _exec_addrn(int argc, char** argv, const char* unparsed) {
    if (argc > 1) {
        // We don't take any arguments.
        cmd_help_display(&cmds_devaddr_n_entry, HELP_DISP_USAGE);
        return (-1);
    }
    // Try to turn the power on
    pdo_request_pwr_on(true);
    int retval = 0;
    _addr++;
    pdo_addr_set(_addr);
    if (ERRORNO) {
        retval = -1;
        goto _finally;
    }
    // Display the address
    shell_printf("%05X\n", _addr);

_finally:
    // Try to turn the power off
    pdo_request_pwr_on(false);

    return (retval);
}

static int _exec_dump(int argc, char** argv, const char* unparsed) {
    static uint16_t _dump_len = 256; // Display 256 bytes unless told otherwise

    if (argc > 3) {
        // We take 0, 1, or 2 arguments.
        cmd_help_display(&cmds_devdump_entry, HELP_DISP_USAGE);
        return (-1);
    }
    // Try to turn the power on
    pdo_request_pwr_on(true);
    int retval = 0;
    argv++; // Skip the command name
    argc--;
    if (argc > 0) {
        uint32_t saddr = _addr;
        uint32_t len = _dump_len;
        // First Arg is START (HEX or '.').
        bool valid = _get_val(&saddr, *argv, 0x7FFFF, true, "hex address");
        if (!valid) {
            retval = -1;
            goto _finally;
        }
        argc--;
        argv++;
        if (argc > 0) {
            valid = _get_val(&len, *argv, 1024, false, "length");
            if (!valid) {
                retval = -1;
                goto _finally;
            }
            argc--;
            argv++;
        }
        if (saddr != _addr) {
            // The address is different, set the address on the device.
            _addr = saddr;
            pdo_addr_set(_addr);
            if (ERRORNO) {
                retval = -1;
                goto _finally;
            }
        }
        _dump_len = len;
    }
    int len = 0;
    uint8_t v[16];
    while (len < _dump_len) {
        // Read and display rows of up to 16 bytes
        //
        // display the address
        shell_printf("%05X  ", _addr);
        int i, j;
        for (i = 0; i < 16; i++) {
            j = i;
            // Read the data from the device
            v[i] = pdo_data_get();
            if (ERRORNO) {
                retval = -1;
                goto _finally;
            }
            // Print the HEX of the value (ASCII comes later)
            shell_printf("%02X ", v[i]);
            len++;
            _addr++;
            pdo_addr_set(_addr);
            if (ERRORNO) {
                retval = -1;
                goto _finally;
            }
            if (len == _dump_len) {
                i++;
                break; // We've reached the number of bytes requested.
            }
        }
        // Fill any gap out to 16 values
        for (;i < 16; i++) {
            shell_printf("   ");
        }
        // Display the ASCII
        shell_printf("  ");
        for (i = 0; i <= j; i++) {
            unsigned char c = (v[i] < 0x20 || v[i] > 0x7F ? '\200' : v[i]);
            shell_printf("%c ", c);
        }
        shell_printf("\n");
    };
_finally:
    // Try to turn the power off
    pdo_request_pwr_on(false);

    return (retval);
}

static int _exec_dpwr(int argc, char** argv, const char* unparsed) {
    progdev_pwr_mode_t pm;

    if (argc > 2) {
        // We only take a single argument.
        cmd_help_display(&cmds_devpwr_entry, HELP_DISP_USAGE);
        return (-1);
    }
    else if (argc > 1) {
        // Argument is 'A' or bool (ON/TRUE/YES/1 | <anything-else>) to set flag
        if (strcasecmp(argv[1], "A") == 0) {
            pdo_pwr_mode(PDPWR_AUTO);
        }
        else {
            bool b = bool_from_str(argv[1]);
            pm = (b ? PDPWR_ON : PDPWR_OFF);
            pdo_pwr_mode(pm);
        }
    }
    pm = pdo_pwr_mode_get();
    char* modestr = (pm == PDPWR_OFF ? "PM_OFF" : (pm == PDPWR_ON ? "PM_ON" : "PM_AUTO"));
    shell_printf("Power Mode: %s  Device Power: %s\n", modestr, (pdo_pwr_is_on() ? "ON" : "OFF"));

    return (0);
}

static int _exec_rd(int argc, char** argv, const char* unparsed) {
    if (argc > 2) {
        // We only take 0 or 1 argument.
        cmd_help_display(&cmds_devrd_entry, HELP_DISP_USAGE);
        return (-1);
    }
    // Try to turn the power on
    pdo_request_pwr_on(true);
    int retval = 0;
    _repeat = false; // using this command with other than 'R' stops any repeat operation.
    _rptop = RPT_NONE;  // no repeat unless enabled below
    if (argc > 1) {
        // The arg is the address to use (hex) or 'R' to do a repetitive read operation.
        if (tolower(*argv[1]) == 'r') {
            _rptop = RPT_RD_DATA;
            _repeat = true;
        }
        else {
            // Get an address.
            char* addrstr = argv[1];
            if (!_get_addr(addrstr)) {
                // _get_addr tells the user the address wasn't valid - just exit.
                retval = -1;
                goto _finally;
            }
        }
    }
    // If they don't want a repeated read, cancel any op-in-progress
    if (!_repeat) {
        if (_rptdlyip) {
            scheduled_msg_cancel2(MSG_EXEC, _repeat_handler);
        }
    }
    // Read the data
    uint8_t data = pdo_data_get();
    if (ERRORNO) {
        retval = -1;
        goto _finally;
    }
    // Display the address and data
    shell_printf("%05X %02X\n", _addr, data);
    if (_repeat) {
        // They want a repeated read, kick it off
        _repeat_handler(NULL);
    }
_finally:
    // Try to turn the power off
    pdo_request_pwr_on(false);

    return (retval);
}

static int _exec_nrd(int argc, char** argv, const char* unparsed) {
    if (argc > 1) {
        // We don't take any arguments.
        cmd_help_display(&cmds_devrd_n_entry, HELP_DISP_USAGE);
        return (-1);
    }
    // Try to turn the power on
    pdo_request_pwr_on(true);
    int retval = 0;
    _rptop = RPT_RD_DATA;
    _addr++;
    pdo_addr_set(_addr);
    if (ERRORNO) {
        retval = -1;
        goto _finally;
    }
    // Read the data
    uint8_t data = pdo_data_get();
    if (ERRORNO) {
        retval = -1;
        goto _finally;
    }
    // Display the address and data
    shell_printf("%05X %02X\n", _addr, data);

_finally:
    // Try to turn the power off
    pdo_request_pwr_on(false);

    return (retval);
}

static int _exec_wr(int argc, char** argv, const char* unparsed) {
    int arg = 1;
    if (argc < 2 || argc > 3) {
        // We only take 1 or 2 arguments: [addr] data | R
        cmd_help_display(&cmds_devwr_entry, HELP_DISP_USAGE);
        return (-1);
    }
    // using this command with other than 'R' stops any repeat operation.
    // Try to turn the power on
    pdo_request_pwr_on(true);
    int retval = 0;
    _rptop = RPT_NONE;
    _repeat = false;
    if (argc > 2) {
        // The additional arg is the address to use (hex).
        argc--;
        char* addrstr = argv[arg++];
        // Get an address.
        if (!_get_addr(addrstr)) {
            // _get_addr tells the user the address wasn't valid - just exit.
            retval = -1;
            goto _finally;
        }
    }
    if (tolower(*argv[arg]) == 'r') {
        _rptop = RPT_WR_DATA;
        _repeat = true;
    }
    else {
        // Get the data to write
        bool success;
        uint8_t data = (uint16_t)uint_from_hexstr(argv[arg], &success);
        if (!success) {
            shell_printf("Value error - '%s' is not a valid hex byte.\n", argv[1]);
            retval = -1;
            goto _finally;
        }
        _data = data;
    }
    // If they don't want a repeated wite, cancel any op-in-progress
    if (!_repeat) {
        if (_rptdlyip) {
            scheduled_msg_cancel2(MSG_EXEC, _repeat_handler);
        }
    }
    // Write the data
    pdo_data_set(_data);
    if (ERRORNO) {
        retval = -1;
        goto _finally;
    }
    if (_repeat) {
        // They want a repeated write, kick it off
        _repeat_handler(NULL);
    }

_finally:
    // Try to turn the power off
    pdo_request_pwr_on(false);

    return (retval);
}

static int _exec_nwr(int argc, char** argv, const char* unparsed) {
    if (argc != 2) {
        // We only take 1 argument: data
        cmd_help_display(&cmds_devwr_n_entry, HELP_DISP_USAGE);
        return (-1);
    }
    // Try to turn the power on
    pdo_request_pwr_on(true);
    int retval = 0;
    _rptop = RPT_WR_DATA;
    _addr++;
    pdo_addr_set(_addr);
    if (ERRORNO) {
        retval = -1;
        goto _finally;
    }
    // Get the data to write
    bool success;
    uint8_t data = (uint16_t)uint_from_hexstr(argv[1], &success);
    if (!success) {
        shell_printf("Value error - '%s' is not a valid hex byte.\n", argv[1]);
        retval = -1;
        goto _finally;
    }
    _data = data;
    // Write the data
    pdo_data_set(_data);
    if (ERRORNO) {
        retval = -1;
        goto _finally;
    }

_finally:
    // Try to turn the power off
    pdo_request_pwr_on(false);

    return (retval);
}

const cmd_handler_entry_t cmds_addrtosect_entry = {
    _exec_atos,
    4,
    "pdatos",
    "addr(hex)",
    "Convert an address to a Device Sector#.",
};

const cmd_handler_entry_t cmds_devaddr_entry = {
    _exec_addr,
    4,
    "pdaddr",
    "[addr(hex)|R]",
    "Show the address being used and optionally set it. Repeat setting it (for troubleshooting).",
};

const cmd_handler_entry_t cmds_devaddr_n_entry = {
    _exec_addrn,
    4,
    "pdan",
    NULL,
    "Advance the address.",
};

const cmd_handler_entry_t cmds_devdump_entry = {
    _exec_dump,
    4,
    "pddump",
    "[[addr(hex)|.] len(dec)]",
    "Dump device data. Optionally specify start address and length.",
};

const cmd_handler_entry_t cmds_devpwr_entry = {
    _exec_dpwr,
    3,
    "pdpwr",
    "A|ON|OFF",
    "Set device Power Mode A|OFF|ON.",
};

const cmd_handler_entry_t cmds_devrd_entry = {
    _exec_rd,
    4,
    "pdrd",
    "[addr(hex)|R]",
    "Read device data from the current or specified address, or start a repeated read.\nUsing this command without 'R' stops any repeated operation.",
};

const cmd_handler_entry_t cmds_devrd_n_entry = {
    _exec_nrd,
    4,
    "pdrn",
    NULL,
    "Advance the address and read device data.",
};

const cmd_handler_entry_t cmds_devwr_entry = {
    _exec_wr,
    4,
    "pdwr",
    "{[addr(hex)] data(hex)}|R",
    "Write device data to the current or specified address, or start a repeated write.\nUsing this command without 'R' stops any repeated operation.",
};

const cmd_handler_entry_t cmds_devwr_n_entry = {
    _exec_nwr,
    4,
    "pdwn",
    "data(hex)",
    "Advance the address and write device data.",
};


void pdcmds_minit(void) {
    cmd_register(&cmds_addrtosect_entry);
    cmd_register(&cmds_devaddr_entry);
    cmd_register(&cmds_devaddr_n_entry);
    cmd_register(&cmds_devdump_entry);
    cmd_register(&cmds_devpwr_entry);
    cmd_register(&cmds_devrd_entry);
    cmd_register(&cmds_devrd_n_entry);
    cmd_register(&cmds_devwr_entry);
    cmd_register(&cmds_devwr_n_entry);
}
