/**
 * CMD Command shell - On the terminal.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 */

#include "cmd.h"
#include "system_defs.h"

#include "cmt/cmt.h"

#include "shell/cmds/debug.h"
#include "shell/cmds/devrdwr.h"
#include "shell/term/term.h"
#include "shell/shell.h"

#include "util.h"
#include "pico/printf.h"

#include <string.h>

#define CMD_LINE_MAX_ARGS 64

// Buffer to save the last input line into for recall
static char _cmdline_last[shell_GETLINE_MAX_LEN_];
// Buffer to copy the input line into to be parsed.
static char _cmdline_parsed[shell_GETLINE_MAX_LEN_];

// Command processor declarations
static int _cmd_cls(int argc, char** argv, const char* unparsed);
static int _cmd_help(int argc, char** argv, const char* unparsed);
static int _cmd_keys(int argc, char** argv, const char* unparsed);
static int _cmd_proc_status(int argc, char** argv, const char* unparsed);
static void _process_line(char* line);

// Command processors framework
static const cmd_handler_entry_t _cmd_cls_entry = {
    // Clear Screen
    _cmd_cls,
    3,
    "cls",
    NULL,
    "Clear the terminal screen.\n",
};
static const cmd_handler_entry_t _cmd_help_entry = {
    _cmd_help,
    1,
    "help",
    "[-a|--all] [command_name [command_name...]]",
    "List of commands or information for a specific command(s).\n  -a|--all : Display hidden commands.\n",
};
static const cmd_handler_entry_t _cmd_keys_entry = {
    _cmd_keys,
    4,
    "keys",
    "",
    "List of the keyboard control key actions.\n",
};
static const cmd_handler_entry_t _cmd_proc_status_entry = {
    _cmd_proc_status,
    3,
    ".ps",
    "",
    "Display process status per second.\n",
};

/**
 * @brief List of Command Handlers
 */
static const cmd_handler_entry_t* _command_entries[] = {
    & cmds_debug_entry,             // .debug - 'DOT' commands come first
    & _cmd_proc_status_entry,       // .ps
    & cmds_devaddr_entry,           // Device Address
    & cmds_devaddr_n_entry,         // Device Address next
    & _cmd_cls_entry,               // Clear Screen
    & cmds_devpwr_entry,            // Device Power
    & cmds_devrd_entry,             // Device Read
    & cmds_devrd_n_entry,           // Device Read next
    & cmds_devwr_entry,             // Device Write
    & cmds_devwr_n_entry,           // Device Write next
    & _cmd_help_entry,
    & _cmd_keys_entry,
    ((cmd_handler_entry_t*)0),      // Last entry must be a NULL
};


// Internal (non-command) declarations

static void _hook_keypress();
static void _wakeup();


// Class data

static cmd_state_t _cmd_state = CMD_SNOOZING;
//static term_color_pair_t _scr_color_save;
//static scr_position_t _scr_cursor_position_save;


// Command built-in functions (others are in the 'cmds' directory)

static int _cmd_cls(int argc, char** argv, const char* unparsed) {
    if (argc > 1) {
        cmd_help_display(&_cmd_cls_entry, HELP_DISP_USAGE);
        return (-1);
    }
    term_clear(true);

    return (0);
}

static int _cmd_help(int argc, char** argv, const char* unparsed) {
    const cmd_handler_entry_t** cmds;
    const cmd_handler_entry_t* cmd;
    bool disp_commands = true;
    bool disp_hidden = false;

    argv++;
    if (argc > 1) {
        // They entered an option and/or command names
        if (strcmp("-a", *argv) == 0 || strcmp("--all", *argv) == 0) {
            disp_hidden = true;
            argv++; argc--;
        }
    }
    if (argc > 1) {
        char* user_cmd;
        for (int i = 1; i < argc; i++) {
            cmds = _command_entries;
            user_cmd = *argv++;
            int user_cmd_len = strlen(user_cmd);
            while (NULL != (cmd = *cmds++)) {
                int cmd_name_len = strlen(cmd->name);
                if (user_cmd_len <= cmd_name_len && user_cmd_len >= cmd->min_match) {
                    if (0 == strncmp(cmd->name, user_cmd, user_cmd_len)) {
                        // This command matches
                        disp_commands = false;
                        cmd_help_display(cmd, HELP_DISP_LONG);
                        break;
                    }
                }
            }
            if (disp_commands) {
                shell_printf("Unknown: '%s'\n", user_cmd);
            }
        }
    }
    if (disp_commands) {
        // List all of the commands with thier usage.
        shell_puts("Commands:\n");
        cmds = _command_entries;
        while (NULL != (cmd = *cmds++)) {
            bool dot_cmd = ('.' == *(cmd->name));
            if (!dot_cmd || (dot_cmd && disp_hidden)) {
                cmd_help_display(cmd, HELP_DISP_NAME);
            }
        }
    }

    return (0);
}

static int _cmd_keys(int argc, char** argv, const char* unparsed) {
    if (argc > 1) {
        cmd_help_display(&_cmd_keys_entry, HELP_DISP_USAGE);
        return (-1);
    }
    shell_puts("':'            : While busy, enters command mode for one command.\n");
    shell_puts("^H             : Backspace (same as Backspace key on most terminals).\n");
    shell_puts("^K or Up-Arrow : Recall last command.\n");
    shell_puts("^R             : Refresh the terminal screen.\n");
    shell_puts("^X             : Clear the input line.\n");

    return (0);
}

static void _cmd_ps_print(const proc_status_accum_t* ps, int corenum) {
    int uaf = ONE_SECOND_MS - ps->t_active;
    shell_printf("Core %d: R:%hu UAF:%d LMSG: %2.2X LMT: %llu IS:0x%0.8X\n",
        corenum, ps->retrieved, ps->t_active, uaf, ps->msg_longest, ps->t_msg_longest, ps->interrupt_status);
}

static int _cmd_proc_status(int argc, char** argv, const char* unparsed) {
    if (argc > 1) {
        cmd_help_display(&_cmd_proc_status_entry, HELP_DISP_USAGE);
    }
    proc_status_accum_t ps0, ps1;
    cmt_sm_counts_t smwc;
    cmt_proc_status_sec(&ps0, 0);
    cmt_proc_status_sec(&ps1, 1);
    smwc = scheduled_msgs_waiting();
    _cmd_ps_print(&ps0, 0);
    _cmd_ps_print(&ps1, 1);
    shell_printf("Scheduled messages: %d\n", smwc.total);

    return (0);
}

int cmd_get_value(const char* v, int min, int max) {
    bool success;
    uint8_t sp = (uint8_t)int_from_str(v, &success);
    if (!success) {
        shell_printf("Value error - '%s' is not a number.\n", v);
        return (INT_MIN);
    }
    if (sp < min || sp > max) {
        shell_puts("Value must be from %d to %d.\n");
        return (INT_MIN);
    }
    return (sp);
}


// Internal functions

static void _cmd_attn_handler(cmt_msg_t* msg) {
    // Called when MSG_CMD_KEY_PRESSED is received
    // We should be in a state where we are looking to be woken up
    // or building up a line to process.
    char c = msg->data.c;
    if (CMD_SNOOZING == _cmd_state) {
        // See if the char is 'wakeup'
        if (CMD_WAKEUP_CHAR == c) {
            _wakeup();
        }
    }
}

void _handle_cc_recall_last(char c) {
    // ^K can be typed to put the last command entered on the current input line.
    shell_getline_append(_cmdline_last);
}

bool _handle_es_recall_last(sescseq_t escseq, const char* escstr) {
    // Up-Arrow (ESC[A) can be typed to put the last command entered on the current input line.
    shell_getline_append(_cmdline_last);
    return (true);
}

/**
 * @brief Registered to handle ^R to re-initialize the terminal.
 *
 * @param c Should be ^R
 */
void _handle_cc_reinit_terminal(char c) {
    // ^R can be typed if the terminal gets messed up or is connected after system has started.
    // This re-initializes the terminal.
    cmt_msg_t msg;
    cmt_msg_init(&msg, MSG_CMD_INIT_TERMINAL);
    msg.data.c = c;
    postAPPMsg(&msg);
}

void _notified_of_keypress() {
    // A character is available from the terminal. Getting called clears the registration.
    //
    // See if it's our wakeup char. If so, post a message to kick off our command processing.
    // If not, throw it away.
    int ci = term_getc();
    while (ci >= 0) {
        char c = (char)ci;
        if (CMD_WAKEUP_CHAR == c) {
            // Post a message to our loop with the character if we can read one (should be able to).
            cmt_msg_t msg;
            cmt_msg_init(&msg, MSG_CMD_KEY_PRESSED);
            msg.data.c = c;
            postAPPMsg(&msg);
            // We don't re-register, as something else handles the incoming chars until we go idle again.
            return;
        }
        else {
            shell_handle_control_character(c); // This might not be a control char, but that's okay.
        }
        // If it's not our wake char, read again. When no chars are ready, the call returns -1.
        ci = term_getc();
    }
    // If we get here we need to re-register so we are notified when another character is ready.
    _hook_keypress();
}

static void _hook_keypress() {
    // Look for our wakeup char being sent from the terminal.
    term_register_notify_on_input(_notified_of_keypress);
}

static void _process_line(char* line) {
    char* argv[CMD_LINE_MAX_ARGS];
    memset(argv, 0, sizeof(argv));

    _cmd_state = CMD_PROCESSING_LINE;

    shell_puts("\n");

    // Copy the line into a buffer for parsing
    strcpy(_cmdline_parsed, line);
    strcpy(_cmdline_last, line);

    int argc = parse_line(_cmdline_parsed, argv, CMD_LINE_MAX_ARGS);
    char* user_cmd = argv[0];
    int user_cmd_len = strlen(user_cmd);
    bool command_matched = false;

    if (user_cmd_len > 0) {
        const cmd_handler_entry_t** cmds = _command_entries;
        const cmd_handler_entry_t* cmd;

        while (NULL != (cmd = *cmds++)) {
            int cmd_name_len = strlen(cmd->name);
            if (user_cmd_len <= cmd_name_len && user_cmd_len >= cmd->min_match) {
                if (0 == strncmp(cmd->name, user_cmd, user_cmd_len)) {
                    // This command matches
                    command_matched = true;
                    _cmd_state = CMD_EXECUTING_COMMAND;
                    cmd->cmd(argc, argv, line);
                    break;
                }
            }
        }
        if (!command_matched) {
            shell_printf("Command not found: '%s'. Try 'help'.\n", user_cmd);
        }
    }
    // If we aren't currently busy, get another line from the user.
    if (!false) {
        // Get a command from the user...
        _cmd_state = CMD_COLLECTING_LINE;
        shell_printf("%c", CMD_PROMPT);
        shell_getline(_process_line);
    }
    else {
        cmd_activate(true);
    }
}

static void _wakeup() {
    // Wakeup the command processor. Change state to building line.
    _cmd_state = CMD_COLLECTING_LINE;
    //term_cursor_moveto(shell_scroll_end_line_get(), 1);
    shell_use_cmd_color();
    putchar('\n');
    putchar(CMD_PROMPT);
    term_cursor_on(true);
    // Get a command from the user...
    shell_getline(_process_line);
}


// Public functions

/**
 * This is typically called by the application when wants the user to have the
 * command processor (true) or when it needs to collect and process input (false).
 */
extern void cmd_activate(bool activate) {
    if (activate) {
        _wakeup();
    }
    else {
        if (CMD_SNOOZING != _cmd_state) {
            // Cancel any inprocess 'getline'
            shell_getline_cancel(_notified_of_keypress);
            // Put the terminal back to 'code' state
            term_cursor_on(false);
            shell_use_output_color();
            // go back to Snoozing
            _cmd_state = CMD_SNOOZING;
        }
    }
}

const cmd_state_t cmd_get_state() {
    return _cmd_state;
}

void cmd_help_display(const cmd_handler_entry_t* cmd, const cmd_help_display_format_t type) {
    term_color_pair_t tc = shell_color_get();
    term_color_default();
    if (HELP_DISP_USAGE == type) {
        shell_puts("Usage: ");
    }
    int name_min = cmd->min_match;
    char* name_rest = ((cmd->name) + name_min);
    // Print the minimum required characters bold and the rest normal.
    term_text_bold();
    // shell_printf(format, cmd->name);
    shell_printf("%.*s", name_min, cmd->name);
    term_text_normal();
    // See if this is an alias for another command...
    bool alias = (CMD_ALIAS_INDICATOR == *cmd->usage);
    if (!alias) {
        shell_printf("%s %s\n", name_rest, cmd->usage);
        if (HELP_DISP_LONG == type || HELP_DISP_USAGE == type) {
            shell_printf("  %s\n", cmd->description);
        }
    }
    else {
        const char* aliased_for_name = ((cmd->usage) + 1);
        shell_printf("%s  Alias for: %s\n", name_rest, aliased_for_name);
        if (HELP_DISP_NAME != type) {
            // Find the aliased entry
            const cmd_handler_entry_t* aliased_cmd = NULL;
            const cmd_handler_entry_t* cmd_chk;
            const cmd_handler_entry_t** cmds = _command_entries;
            while (NULL != (cmd_chk = *cmds++)) {
                if (strcmp(cmd_chk->name, aliased_for_name) == 0) {
                    aliased_cmd = cmd_chk;
                    break;
                }
            }
            if (aliased_cmd) {
                // Put the terminal colors back, and call this again with the aliased command
                term_color_fg(tc.fg);
                term_color_bg(tc.bg);
                cmd_help_display(aliased_cmd, type);
            }
        }
    }
    term_color_fg(tc.fg);
    term_color_bg(tc.bg);
}


void cmd_module_init() {
    _cmd_state = CMD_SNOOZING;
    // Register the control character handlers.
    shell_register_control_char_handler(CMD_REINIT_TERM_CHAR, _handle_cc_reinit_terminal);
    shell_register_control_char_handler(CMD_RECALL_LAST_CHAR, _handle_cc_recall_last);
    shell_register_esc_seq_handler(SES_KEY_ARROW_UP, _handle_es_recall_last);
    //
    // Register our message handler
    cmt_msg_hdlr_add(MSG_CMD_KEY_PRESSED, _cmd_attn_handler);
    //
    // Hook keypress looking for a ':' to wake us up.
    _hook_keypress();
}
