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

#include "picoutil.h"

#include "shell/include/shell.h"
#include "shell/cmd/cmd_t.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>


const cmd_handler_entry_t cmds_bootldr_entry;


static int _exec_bootldr(int argc, char** argv, const char* unparsed) {
    if (argc > 1) {
        // No arguments.
        cmd_help_display(&cmds_bootldr_entry, HELP_DISP_USAGE);
        return (-1);
    }
    shell_printf("Rebooting to LOADER...\n");
    boot_to_bootsel();
    // We should never get here
    return (0);
}

const cmd_handler_entry_t cmds_bootldr_entry = {
    _exec_bootldr,
    7,
    "bootldr",
    NULL,
    "Reboot to the UF2 loader."
};


void picocmds_minit(void) {
    cmd_register(&cmds_bootldr_entry);
}
