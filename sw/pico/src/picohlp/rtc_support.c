/**
 * RTC_SUPPORT Provides common RTC methods that can be used whether or not a Pico
 * that has an actual RTC is being used. (RP2350 doesn't have an RTC)
 *
 * The method in this module use the actual Pico RTC if it is available, or
 * maintain a Virtual/Simulated RTC if it isn't. The Virtual RTC is updated by the
 * system tick (time since reset) as needed.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 *  Portions - Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *             SPDX-License-Identifier: BSD-3-Clause
 *
*/

#include "rtc_support.h"


#include "pico/types.h" // 'uint' and other standard types

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// ====================================================================
// Data Section
// ====================================================================

static bool _initialized;

// ====================================================================
// Local/Private Method Declarations
// ====================================================================


// ====================================================================
// Local/Private Methods
// ====================================================================


// ====================================================================
// Public Methods
// ====================================================================


// ====================================================================
// Initialization/Start-Up Methods
// ====================================================================

void rtc_support_module_init() {
    if (_initialized) {
        return;     // Already initialized, just return.
    }
}
