/**
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef DISPOPS_H_
#define DISPOPS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

extern int disp_write(uint8_t data);

extern bool disp_cmd_op_start();

extern bool disp_data_op_start();

extern void disp_op_end();

extern void disp_reset();

#ifdef __cplusplus
}
#endif
#endif // DISPOPS_H_

