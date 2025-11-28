/**
 * @brief Methods that are implemented for either I2C or SPI.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef OLED1106_H_
#define OLED1106_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

extern void oled1106_send_cmd(uint8_t cmd);

extern void oled1106_send_cmdx(uint8_t* cmds, size_t num);


#ifdef __cplusplus
}
#endif
#endif // OLED1106_H_
