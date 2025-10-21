/**
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FONT_9_10_H_H_
#define _FONT_9_10_H_H_
#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

#define LOWBYTE(v)   ((unsigned char) (v))
#define HIGHBYTE(v)  ((unsigned char) (((unsigned int) (v)) >> 8))

#define FONT_WIDTH 9
#define FONT_HEIGHT 10
#define FONT_BIT_MASK 0x01FF
extern const uint16_t Font_Table[];

#ifdef __cplusplus
}
#endif
#endif // _FONT_9_10_H_H_
