/**
 * Programmable Device Control functionality.
 *
 * Methods to control the POWER-ENABLE, WRITE-ENABLE, and READ-ENABLE signals, and to
 * set the device data address and increment the address.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef PDOPS_H_
#define PDOPS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "system_defs.h"

#include <stdbool.h>


/**
 * @brief Set the device data location address.
 *
 * This stores the address into the Device Address Latches/Counters. The devices use/require:
 *      A. 128K device: 17 Addr Bits
 *      B. 256K device: 18 Addr Bits
 *      C. 512K device: 19 Addr Bits
 * This always sets 19 Addr Bits.
 *
 * @param addr The address to set
 */
extern void pdo_addr_set(uint32_t addr);

/**
 * @brief Read a byte of data from the device into the input buffer, then into the Pico.
 *
 * This performs two operations to get a byte from the device. It controls the device
 * chip-select and output-enable to read the byte into the input register, then it reads
 * the input register into the pico.
 *
 * The address must have been set using `pdo_addr_set`.
 *
 * @see pdo_addr_set(uint32_t addr)
 *
 * @return uint8_t 8-bit byte from the device
 */
extern uint8_t pdo_data_get();

/**
 * @brief Set the data into the output buffers for the device, then to the device.
 *
 * This performs two operations to write a byte to the device. It writes the byte to the
 * data buffer, then it controls the chip-select and write-enable on the device to write
 * the byte to the device.
 *
 * The address must have been set using `pdo_addr_set`.
 *
 * @see pdo_addr_set(uint32_t addr)
 *
 * @param data 8-bit data byte
 */
extern void pdo_data_set(uint8_t data);

/**
 * @brief Turn the Programmable-Device Power ON/OFF.
 *
 * @param on True:ON, False:OFF
 */
static inline void pdo_pwr_on(bool on) {
    gpio_put(OP_DEVICE_PWR, on);
}

/**
 * @brief Get the state of the Programmable-Device Power.
 *
 * @return true ON
 * @return false OFF
 */
static inline bool pdo_pwr_is_on() {
    return (gpio_get(OP_DEVICE_PWR) != 0);
}

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void pdo_minit();

#ifdef __cplusplus
}
#endif
#endif // PDOPS_H_
