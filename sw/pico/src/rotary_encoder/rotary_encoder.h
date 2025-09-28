/**
 * @brief rotary encoder decoding functionality.
 * @ingroup ui
 *
 * This provides input from the rotary encoder.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef ROTARY_ENQ_H_
#define ROTARY_ENQ_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <pico/types.h>

/**
 * @brief Current position count of the rotary encoder.
 *
 * @return int32_t Current position count
 */
extern int32_t re_count(void);

/**
 * @brief Last position delta of the rotary encoder.
 * 
 * @return int16_t Delta of last change
 */
extern int16_t re_delta(void);

extern void re_turn_irq_handler(uint gpio, uint32_t events);

/**
 * @brief Initialize the rotary encoder decode module.
 * @ingroup ui
 */
extern void rotary_encoder_module_init();

#ifdef __cplusplus
    }
#endif
#endif // _rotary_ENQ_H
