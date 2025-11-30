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

#include "app_t.h"
#include "system_defs.h"

#include <stdbool.h>

/** @brief Programmable-Device Sector Size (4K) */
#define PD_SECTSIZE (4096)

/**
 * @brief Programmable-Device Power Mode: One of OFF, ON, AUTO
 * @ingroup ProgDev
 *
 * The OFF and ON modes directly turn the Device Power off and on.
 * The AUTO mode indicates to the operating functions that they can
 * control the power as needed - normally leaving it off, but turning
 * it on for the duration of operations that need the power on.
 */
typedef enum progdev_pwr_mode_ {
    PDPWR_OFF = 0,
    PDPWR_ON = 1,
    PDPWR_AUTO = 2
} progdev_pwr_mode_t;

/**
 * @brief Set the device data location address.
 * @ingroup ProgDev
 *
 * This stores the address into the Device Address Latches/Counters. The devices use/require:
 *      A. 128K device: 17 Addr Bits (00000-1FFFF)
 *      B. 256K device: 18 Addr Bits (00000-3FFFF)
 *      C. 512K device: 19 Addr Bits (00000-7FFFF)
 * This always sets 19 Addr Bits.
 *
 * @param addr The address to set
 */
extern void pdo_addr_set(uint32_t addr);

/**
 * @brief Read a byte of data from the device into the input buffer, then into the Pico.
 * @ingroup ProgDev
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
 * @ingroup ProgDev
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
 * @brief Get the state of the Programmable-Device Power.
 * @ingroup ProgDev
 *
 * @return true ON
 * @return false OFF
 */
static inline bool pdo_pwr_is_on() {
    return (gpio_get(OP_DEVICE_PWR) != 0);
}

/**
 * @brief Set the Programmable-Device Power Mode: OFF, ON, AUTO.
 * @ingroup ProgDev
 *
 * When power is turned off, the PWR- signal is set LOW to avoid back-powering the
 * power-controlled portion of the circuit. When it is turned on, the PWR- signal
 * is set HIGH to avoid driving the PD data bus when not intended.
 *
 * When set to AUTO, the power to the device is normally kept off, but is turned
 * on as needed to perform operations.
 *
 * @param on True:ON, False:OFF
 */
extern void pdo_pwr_mode(progdev_pwr_mode_t mode);

/**
 * @brief Get the current Programmable-Device Power Mode.
 * @ingroup ProgDev
 *
 * @return progdev_pwr_mode_t
 */
extern progdev_pwr_mode_t pdo_pwr_mode_get();

/**
 * @brief Request that the Programmable-Device be powered ON/OFF.
 * @ingroup ProgDev
 *
 * This is a request, because the Power Mode controls what can be done. If the
 * Power Mode is ON or AUTO and the request is ON, the power will be turned ON. If
 * the Power Mode is OFF or AUTO and the request is OFF, the power will be turned
 * OFF.
 *
 * @param on true to turn on, false to turn off
 * @return true The request succeeded
 * @return false The request was denied
 */
extern bool pdo_request_pwr_on(bool on);

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 * @ingroup ProgDev
 *
 */
extern void pdo_minit();

#ifdef __cplusplus
}
#endif
#endif // PDOPS_H_
