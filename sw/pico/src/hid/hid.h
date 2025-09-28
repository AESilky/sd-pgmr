/**
 * @brief Human Interface Device.
 * @ingroup hid
 *
 * Display status and provide the human interface.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef HID_H_
#define HID_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Starts the HID.
 * @ingroup hid
 *
 * This should be called after the messaging system is up and running.
 */
extern void start_hid(void);

#ifdef __cplusplus
    }
#endif
#endif // HID_H_
