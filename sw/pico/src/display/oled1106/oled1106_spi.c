/**
 * Copyright 2023-25 AESilky
 * Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "display.h"
#include "dispops.h"

#include <stdio.h>

void oled1106_send_cmd(uint8_t cmd) {
    disp_cmd_op_start();
    disp_write(cmd);
    disp_op_end();
}

void oled1106_send_cmdx(uint8_t* cmds, size_t num) {
    disp_cmd_op_start();
    for (size_t i = 0; i < num; i++) {
        disp_write(*(cmds + i));
    }
    disp_op_end();
}


