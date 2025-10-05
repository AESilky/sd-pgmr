/**
 * Programmable Device operations.
 *
 * Maintains the image to be programmed and provides operations to load/modify/save it and
 * perform operations on the device to be programmed.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef PROG_DEVICE_H_
#define PROG_DEVICE_H_
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void pd_module_init();

#ifdef __cplusplus
}
#endif
#endif // PROG_DEVICE_H_
