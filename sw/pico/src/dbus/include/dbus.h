/**
 * Data Bus operations.
 *
 * Provide functions that control 8 bidirectional data pins and a RD pin and WR pin.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef DBUS_H_
#define DBUS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "system_defs.h"

#include "pico/types.h" // 'uint' and other standard types

#include <stdbool.h>
#include <stdint.h>


/**
 * @brief Check if the Data Bus direction is OUT.
 *
 * @return true It is OUT
 * @return false It is IN
 */
static inline bool dbus_is_out() {
    return gpio_get_dir(DATA0); // We check DATA-0. All DATA bits direction are set as one.
}


/**
 * @brief Read the value of the DATA Bus.
 *
 * @return uint8_t The value read.
 */
    extern uint8_t dbus_rd();

/**
 * @brief Set the direction of the DATA Bus to inbound.
 *
 * This is used to put the DATA Bus to the inbound direction, which is safer
 * than leaving it as outputs.
 */
static inline void dbus_set_in() {
    // Set the DATA Bus to inbound
    gpio_set_dir_in_masked(DATA_BUS_MASK);
}

/**
 * @brief Set the direction of the DATA Bus to inbound.
 *
 * This is used to put the DATA Bus to the inbound direction, which is safer
 * than leaving it as outputs.
 */
static inline void dbus_set_out() {
    // Set the DATA Bus to outbound
    gpio_set_dir_out_masked(DATA_BUS_MASK);
}

/**
 * @brief Set the PDATA bus to be outputs and set the value on the bus.
 *
 * @param data Data value to put on the bus
 */
extern void dbus_wr(uint8_t data);


/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void dbus_minit();

#ifdef __cplusplus
}
#endif
#endif // DBUS_H_
