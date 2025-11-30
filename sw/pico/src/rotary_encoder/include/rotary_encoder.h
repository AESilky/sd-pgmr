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

#include "pico/types.h"

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

/**
 * @brief Time between previous rotary change and the current change.
 *
 * @return int32_t Delta-T in ms
 */
extern int32_t re_tdelta();

/**
 * @brief Time of the last change of the rotary dial.
 *
 * @return int32_t Timestamp in ms
 */
extern int32_t re_tlast();

/**
 * @brief The change over time of the encoder.
 *
 * @return int32_t The change.
 */
extern int32_t re_velocity();

/**
 * @brief Handler for the Rotary Encoder shaft.
 *
 * @param phase The EVEN indicates to request the value, ODD to fetch the value.
 */
extern void re_turn_handler(uint phase);

/**
 * @brief Initialize the rotary encoder decode module.
 * @ingroup ui
 */
extern void re_minit();

#ifdef __cplusplus
    }
#endif
#endif // _rotary_ENQ_H
