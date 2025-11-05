/**
 * Commends: Debug.
 *
 * Shell command to display, set, reset the debug flag.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */

#include "devrdwr.h"
#include "cmt/cmt.h"
#include "shell/shell.h"
#include "util.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "deviceops/pdops.h"

#define DDRDWR_REPEAT_MS 10

typedef enum RPTOP_ {
    RPT_NONE,
    RPT_ADDR_SET,
    RPT_WR_DATA,
    RPT_RD_DATA
} rptop_t;

static uint32_t _addr;
static uint8_t _data;
static bool _repeat;
static rptop_t _rptop;
static bool _rptdlyip; // True if a repeat delay has been scheduled and not received.

static int _get_addr(char* addrstr) {
    bool success;
    uint32_t addr = (uint32_t)uint_from_hexstr(addrstr, &success);
    if (!success) {
        shell_printf("Value error - '%s' is not a valid hex address.\n", addrstr);
        return (-1);
    }
    if (addr != _addr) {
        // The address is different, set the address on the device.
        _addr = addr;
        pdo_addr_set(_addr);
    }
    return (0);
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

static int _exec_addr(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    if (argc > 2) {
        // We only take 0 or 1 argument.
        cmd_help_display(&cmds_devaddr_entry, HELP_DISP_USAGE);
        return (-1);
    }
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
            if (_get_addr(addrstr) < 0) {
                // _get_addr tells the user the address wasn't valid - just exit.
                return (-1);
            }
        }
    }
    // Display the address
    shell_printf("%6.6X\n", _addr);
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

    return (retval);
}

static int _exec_naddr(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    if (argc > 1) {
        // We don't take any arguments.
        cmd_help_display(&cmds_devaddr_n_entry, HELP_DISP_USAGE);
        return (-1);
    }
    _addr++;
    pdo_addr_set(_addr);
    // Display the address
    shell_printf("%6.6X\n", _addr);

    return (retval);
}

static int _exec_dpwr(int argc, char** argv, const char* unparsed) {
    if (argc > 2) {
        // We only take a single argument.
        cmd_help_display(&cmds_devpwr_entry, HELP_DISP_USAGE);
        return (-1);
    }
    else if (argc > 1) {
        // Argument is bool (ON/TRUE/YES/1 | <anything-else>) to set flag
        bool b = bool_from_str(argv[1]);
        pdo_pwr_on(b);
    }
    shell_printf("Device Power: %s\n", (pdo_pwr_is_on() ? "ON" : "OFF"));

    return (0);
}

static int _exec_rd(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    if (argc > 2) {
        // We only take 0 or 1 argument.
        cmd_help_display(&cmds_devrd_entry, HELP_DISP_USAGE);
        return (-1);
    }
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
            if (_get_addr(addrstr) < 0) {
                // _get_addr tells the user the address wasn't valid - just exit.
                return (-1);
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
    // Display the address and data
    shell_printf("%6.6X %2.2X\n", _addr, data);
    if (_repeat) {
        // They want a repeated read, kick it off
        _repeat_handler(NULL);
    }

    return (retval);
}

static int _exec_nrd(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    if (argc > 1) {
        // We don't take any arguments.
        cmd_help_display(&cmds_devrd_n_entry, HELP_DISP_USAGE);
        return (-1);
    }
    _rptop = RPT_RD_DATA;
    _addr++;
    pdo_addr_set(_addr);
    // Read the data
    uint8_t data = pdo_data_get();
    // Display the address and data
    shell_printf("%6.6X %2.2X\n", _addr, data);

    return (retval);
}

static int _exec_wr(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    int arg = 1;
    if (argc < 2 || argc > 3) {
        // We only take 1 or 2 arguments: [addr] data | R
        cmd_help_display(&cmds_devwr_entry, HELP_DISP_USAGE);
        return (-1);
    }
    // using this command with other than 'R' stops any repeat operation.
    _rptop = RPT_NONE;
    _repeat = false;
    if (argc > 2) {
        // The additional arg is the address to use (hex).
        argc--;
        char* addrstr = argv[arg++];
        // Get an address.
        if (_get_addr(addrstr) < 0) {
            // _get_addr tells the user the address wasn't valid - just exit.
            return (-1);
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
            return (-1);
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
    if (_repeat) {
        // They want a repeated write, kick it off
        _repeat_handler(NULL);
    }

    return (retval);
}

static int _exec_nwr(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    if (argc != 2) {
        // We only take 1 argument: data
        cmd_help_display(&cmds_devwr_n_entry, HELP_DISP_USAGE);
        return (-1);
    }
    _rptop = RPT_WR_DATA;
    _addr++;
    pdo_addr_set(_addr);
    // Get the data to write
    bool success;
    uint8_t data = (uint16_t)uint_from_hexstr(argv[1], &success);
    if (!success) {
        shell_printf("Value error - '%s' is not a valid hex byte.\n", argv[1]);
        return (-1);
    }
    _data = data;
    // Write the data
    pdo_data_set(_data);

    return (retval);
}

const cmd_handler_entry_t cmds_devaddr_entry = {
    _exec_addr,
    4,
    "addr",
    "[addr(hex)|R]"
    "Show the address being used and optionally set it. Repeat setting it (for troubleshooting).",
};

const cmd_handler_entry_t cmds_devaddr_n_entry = {
    _exec_naddr,
    2,
    "naddr",
    NULL,
    "Advance the address.",
};

const cmd_handler_entry_t cmds_devpwr_entry = {
    _exec_dpwr,
    2,
    "dpwr",
    "[ON|OFF]",
    "Turn device power ON/OFF.",
};

const cmd_handler_entry_t cmds_devrd_entry = {
    _exec_rd,
    2,
    "rd",
    "[addr(hex)|R]",
    "Read device data from the current or specified address, or start a repeated read.\nUsing this command without 'R' stops any repeated operation.",
};

const cmd_handler_entry_t cmds_devrd_n_entry = {
    _exec_nrd,
    2,
    "nrd",
    NULL,
    "Advance the address and read device data.",
};

const cmd_handler_entry_t cmds_devwr_entry = {
    _exec_wr,
    2,
    "wr",
    "{[addr(hex)] data(hex)}|R",
    "Write device data to the current or specified address, or start a repeated write.\nUsing this command without 'R' stops any repeated operation.",
};

const cmd_handler_entry_t cmds_devwr_n_entry = {
    _exec_nwr,
    2,
    "nwr",
    "data(hex)",
    "Advance the address and write device data.",
};
