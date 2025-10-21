/**
 * Commends: Debug.
 *
 * Shell command to display, set, reset the debug flag.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */

#include "debug.h"

#include "shell/shell.h"

#include "debug_support.h"
#include "util.h"

#include <stdbool.h>

static int _debug_cmd_debug(int argc, char** argv, const char* unparsed) {
    if (argc > 2) {
        // We only take a single argument.
        cmd_help_display(&cmds_debug_entry, HELP_DISP_USAGE);
        return (-1);
    }
    else if (argc > 1) {
        // Argument is bool (ON/TRUE/YES/1 | <anything-else>) to set flag
        bool b = bool_from_str(argv[1]);
        debug_mode_enable(b);
    }
    shell_printf("Debug: %s\n", (debug_mode_enabled() ? "ON" : "OFF"));

    return (0);
}


const cmd_handler_entry_t cmds_debug_entry = {
    _debug_cmd_debug,
    2,
    ".debug",
    "[ON|OFF]",
    "Set/reset debug flag.",
};

