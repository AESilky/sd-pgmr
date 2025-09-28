/**
 * Copyright 2023 AESilky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _DISPLAY_SPI_OPS_H_
#define _DISPLAY_SPI_OPS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum _op_cmd_data {
    DISP_OP_CMD,
    DISP_OP_DATA,
} op_cmd_data_t;

extern int display_data_write(uint8_t data);

extern bool display_op_start(op_cmd_data_t cd);

extern void display_op_end();

extern void display_reset();

#ifdef __cplusplus
}
#endif
#endif // _DISPLAY_SPI_OPS_H_

