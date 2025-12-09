/**
 * Commends: Data Bus Operations
 *
 * Shell commands for the Programmable Device.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */

#include "cmds.h"
#include "dbus.h"

#include "include/util.h"

#include "shell/include/shell.h"
#include "shell/cmd/cmd_t.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>


const cmd_handler_entry_t cmds_dbus_data_entry;
const cmd_handler_entry_t cmds_dbus_dir_entry;
const cmd_handler_entry_t cmds_dbus_dlatch_entry;
const cmd_handler_entry_t cmds_dbus_rd_entry;
const cmd_handler_entry_t cmds_dbus_wr_entry;


static int _exec_data(int argc, char** argv, const char* unparsed) {
    if (argc > 2) {
        // We only take 0 or 1 argument.
        cmd_help_display(&cmds_dbus_data_entry, HELP_DISP_USAGE);
        return (-1);
    }
    uint8_t data;
    if (argc > 1) {
        // The arg is the value (hex) to set on the Data Bus.
        bool success;
        data = (uint16_t)uint_from_hexstr(argv[1], &success);
        if (!success) {
            shell_printf("Value error - '%s' is not a valid hex byte.\n", argv[1]);
            return (-1);
        }
        dbus_wr(data);
        shell_printf("DBUS written: %02X\n", data);
    }
    // Display the data from the Data Bus
    data = dbus_rd();
    shell_printf("%02X\n", data);

    return (0);
}

static int _exec_dir(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    if (argc > 2) {
        // We only take 0 or 1 arguments.
        cmd_help_display(&cmds_dbus_dir_entry, HELP_DISP_USAGE);
        return (-1);
    }
    if (argc > 1) {
        if (strcasecmp(argv[1], "O") == 0) {
            // Set to output
            dbus_set_out();
            shell_printf("Data Bus set to OUT\n");
        }
        else if (strcasecmp(argv[1], "I") == 0) {
            // Set to input
            dbus_set_in();
            shell_printf("Data Bus set to IN\n");
        }
        else {
            shell_printferr("Value error: '%s'\n", argv[1]);
            cmd_help_display(&cmds_dbus_dir_entry, HELP_DISP_USAGE);
            return (-1);
        }
    }
    // Display the direction
    char* dstr = (dbus_is_out() ? "OUT" : "IN");
    shell_printf("Data Bus is: %s\n", dstr);

    return (retval);
}

static int _exec_dlatch(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    if (argc > 2) {
        // We only take 0 or 1 arguments.
        cmd_help_display(&cmds_dbus_dlatch_entry, HELP_DISP_USAGE);
        return (-1);
    }
    bool v;
    if (argc > 1) {
        v = bool_from_str(argv[1]);
        gpio_put(OP_DATA_LATCH, v);
        const char* vstr = (v ? "HIGH" : "LOW");
        shell_printf("Set DataLatch: %s\n", vstr);
    }
    // Display the level
    const char* dlstr = (gpio_get(OP_DATA_LATCH) ? "HIGH" : "LOW");
    shell_printf("DataLatch is: %s\n", dlstr);

    return (retval);
}

static int _exec_drd(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    if (argc > 2) {
        // We only take 0 or 1 arguments.
        cmd_help_display(&cmds_dbus_rd_entry, HELP_DISP_USAGE);
        return (-1);
    }
    bool v;
    if (argc > 1) {
        v = bool_from_str(argv[1]);
        gpio_put(OP_DATA_RD, v);
        const char* vstr = (v ? "HIGH" : "LOW");
        shell_printf("Set DRD: %s\n", vstr);
    }
    // Display the level
    const char* drstr = (gpio_get(OP_DATA_RD) ? "HIGH" : "LOW");
    shell_printf("DRD is: %s\n", drstr);

    return (retval);
}

static int _exec_dwr(int argc, char** argv, const char* unparsed) {
    int retval = 0;
    if (argc > 2) {
        // We only take 0 or 1 arguments.
        cmd_help_display(&cmds_dbus_wr_entry, HELP_DISP_USAGE);
        return (-1);
    }
    bool v;
    if (argc > 1) {
        v = bool_from_str(argv[1]);
        gpio_put(OP_DATA_WR, v);
        const char* vstr = (v ? "HIGH" : "LOW");
        shell_printf("Set DWR: %s\n", vstr);
    }
    // Display the level
    const char* drstr = (gpio_get(OP_DATA_WR) ? "HIGH" : "LOW");
    shell_printf("DWR is: %s\n", drstr);

    return (retval);
}

const cmd_handler_entry_t cmds_dbus_data_entry = {
    _exec_data,
    7,
    ".dbusdata",
    "[val(hex)]"
    "Get value from Data Bus. Set value to Data Bus.",
};

const cmd_handler_entry_t cmds_dbus_dir_entry = {
    _exec_dir,
    7,
    ".dbusdir",
    "[I|O]"
    "Show the direction of the Data Bus. Set the direction of the Data Bus.",
};

const cmd_handler_entry_t cmds_dbus_dlatch_entry = {
    _exec_dlatch,
    5,
    ".dlatch",
    "[0|1]",
    "Show the DLATCH state. Set the DLATCH state."
};

const cmd_handler_entry_t cmds_dbus_rd_entry = {
    _exec_drd,
    8,
    ".dbusrdctrl",
    "[0|1]",
    "Show the RD ctrl state. Set the RD ctrl state."
};

const cmd_handler_entry_t cmds_dbus_wr_entry = {
    _exec_dwr,
    8,
    ".dbuswrctrl",
    "[0|1]",
    "Show the WR ctrl state. Set the WR ctrl state."
};



void dbuscmds_minit(void) {
    cmd_register(&cmds_dbus_data_entry);
    cmd_register(&cmds_dbus_dir_entry);
    cmd_register(&cmds_dbus_dlatch_entry);
    cmd_register(&cmds_dbus_rd_entry);
    cmd_register(&cmds_dbus_wr_entry);
}
