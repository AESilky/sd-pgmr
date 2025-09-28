/**
 * Hardware Runtime.
 *
 * Setup for the message loop and idle processing.
 * Also, defines the high-level board functionality that runs
 * on Core-1.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef HWRT_H_
#define HWRT_H_
#ifdef __cplusplus
extern "C" {
#endif

#define HWRT_CORE_NUM 0

/**
 * @brief The Hardware Runtime 0 and 1 define a `core1_main` which is run
 * by the `start_core1` function. That controls the Board (0 or 1) high-level
 * functionality.
 */
extern void core1_main(void);

/**
 * @brief Start the runtime (core 0 (endless) message-loop).
 * @ingroup hwrt
 */
extern void start_hwrt(void);


#ifdef __cplusplus
}
#endif
#endif // HWRT_H_
