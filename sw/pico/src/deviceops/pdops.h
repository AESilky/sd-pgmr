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
 * @brief Increment the address counters.
 */
extern void pdo_addr_inc();

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
 * @brief Assert the device WR_EN signal.
 *
 * @param enable True:Enabled False:Not-Enabled
 */
extern void pdo_wr_en(bool enable);

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void pdo_module_init();

#ifdef __cplusplus
}
#endif
#endif // PDOPS_H_
