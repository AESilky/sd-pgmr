/**
 * Board Initialization and General Functions.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 * This sets up the Pico.
 * It:
 * 1 Configures the GPIO Pins for the proper IN/OUT
 *
 * It provides some utility methods to:
 * 1. Turn the On-Board LED ON/OFF
 * 2. Flash the On-Board LED a number of times
 *
*/
#ifndef Board_H_
#define Board_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "system_defs.h"

#include "pico/mutex.h"

#include <stdbool.h>
#include <stdint.h>

#define SHARED_PRINT_BUF_SIZE 256
extern char shared_print_buf[SHARED_PRINT_BUF_SIZE];

extern const uint LED_PIN;

typedef enum BOARD_OP {
    BDO_NONE = OP8_NONE,
    BDO_ADDR_LOW_LD = OP8_ADDRL_LD,
    BDO_ADDR_MID_LD = OP8_ADDRM_LD,
    BDO_ADDR_HIGH_LD = OP8_ADDRH_LD,
    BDO_PRGMDEV_SEL = OP8_DEV_SEL,
    BDO_DISPLAY_RST = OP8_DISP_RST,
} boardop_t;


/**
 * @brief Alias for a mutex_t pointer. Used to control access to Board Operations.
 */
typedef mutex_t* boptkn_t;

/**
 * @brief Initialize the board
 * @ingroup board
 *
 * This sets up the GPIO for the proper direction (IN/OUT), pull-ups, etc.
 * This calls the init for each of the devices/subsystems.
 * If all is okay, it returns true, else false.
*/
extern int board_init(void);

/**
 * @brief Enable Board Operation signal.
 * @ingroup board
 *
 * This takes one of the 7 board operation signals (or none of them) LOW. These
 * are used for various functions on the board, and the resulting functionality
 * should be understood by the programmer using them.
 *
 * @param bdop One of seven operation signals, or none of them to enable.
 * @param boptkn* Pointer to the Board Operation Token returned from a call to board_op_start().
 */
extern void board_op(boptkn_t boptkn, boardop_t bdop);

/**
 * @brief Attempt to start a board operation that requires control of a board signal.
 * @ingroup board
 *
 * This checks whether a board operation is in progress (by examining a mutex). If none
 * is in progress, this method acquires the mutex and returns true. If the mutex can't
 * be acquired, false is returned.
 *
 * @see board_op_end()
 *
 * @return boptkn Board Operation Token if the operation was started, or NULL if it can't be.
 */
extern boptkn_t board_op_start();

/**
 * @brief End a board operation.
 * @ingroup board
 *
 * This must be called after board operations that require an exclusive board signal are complete.
 * The parameter is the token that was returned by a call to board_op_start().
 *
 * @see board_op_start()
 * @see board_op(boptkn_t, boardop_t)
 *
 * @param boptkn Board Operation Token returned from board_op_start()
 */
extern void board_op_end(boptkn_t boptkn);

/**
 * @brief Get the state of the Rotary Switch
 *
 * @return true The switch is pressed
 * @return false The switch isn't pressed
 */
extern bool rotary_switch_pressed();

/**
 * @brief Get the state of the Command Attention (CMDATTN) Switch
 *
 * @return true The switch is pressed
 * @return false The switch isn't pressed
 */
extern bool cmdattn_switch_pressed();

/**
 * @brief Allow / Don't Allow Diagnostic output.
 *
 * Diagnostic output is from:
 * 1) debug_printf (this is also controlled by the debug flag)
 * 2) error_printf
 * 3) info_printf
 * 4) warn_printf
 *
 * @param enable True to allow, False to not allow
 */
extern void diagout_enable(bool enable);

/**
 * @brief Get the state of the Diagnostic Enabled flag.
 *
 * @see diagout_enable for a list of what this controls.
 *
 * @return true Diagnostic output is enabled
 * @return false Diagnostic output is not enabled
 */
extern bool diagout_is_enabled();


/** @brief Printf like function that includes the datetime and type prefix */
extern void error_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));
/** @brief Printf like function that includes the datetime and type prefix */
extern void info_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));
/** @brief Printf like function that includes the datetime and type prefix */
extern void warn_printf(const char* format, ...) __attribute__((format(_printf_, 1, 2)));

/**
 * @brief Board level (common) PANIC.
 * @ingroup board
 *
 * This should be used in preference to directly using the Pico `panic` to make
 * it better for debugging and common fatal error handling.
 *
 * This attempts to turn the Pico LED on and Error-Print the message before
 * performing the `panic`.
 *
 * @param fmt format string (printf-like)
 * @param ...  printf-like arguments
 */
extern void board_panic(const char* fmt, ...);

#ifdef __cplusplus
}
#endif
#endif // Board_H_
