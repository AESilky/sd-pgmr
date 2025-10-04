/**
 * Hardware Runtime datatypes.
 *
 * Contains the HWRT data types, structures, etc. used by other modules.
 *
 * This is the include file for the 'types' used by CMT (and modules consuming messages).
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/

#include <stdbool.h>

/**
 * @brief Switch IDs
 */
typedef enum _SWITCH_ID_ {
    /** VALUE to indicate no switch pressed */
    SW_NONE     = 0,
    SW_ROTARY   = 1,
    SW_USER     = 2,
} switch_id_t;

/**
 * @brief Information for a switch action.
 * @ingroup curswitch
 */
typedef struct _sw_action_data_ {
    /** Switch ID */
    switch_id_t switch_id;
    /** Switch pressed. Otherwise, released. */
    bool pressed;
    /** Action is a 'repeat' */
    bool repeat;
} switch_action_data_t;

