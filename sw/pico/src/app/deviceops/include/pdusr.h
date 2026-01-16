/**
 * @brief Programmable Device User Operations
 * @file pdusr.h
 * @ingroup device
 *
 * Used by both the command shell and the Rotary+Switch and Display to keep them
 * consistent.
 *
 * Copyright 2025 AESilky
 * SPDX-License-Identifier: MIT License
 */
#ifndef PDUSR_H_
#define PDUSR_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "prog_device.h"

#include <ctype.h>
#include <stdbool.h>


/**
 * @brief Erase complete device.
 * @ingroup usrdevice
 *
 * @param shellout True to output to the shell (in addition to the display)
 * @return bool True if successful.
 */
extern bool pdusr_erase_all(bool shellout);

/**
 * @brief Erase a sector of the device.
 * @ingroup usrdevice
 *
 * @param info Info for the device
 * @param sect The sector number to erase
 * @param shellout True to output to the shell (in addition to the display)
 * @return bool True if successful.
 */
extern bool pdusr_erase_sect(const md_info_t* info, int sect, bool shellout);

/**
 * @brief Programmable Device Info
 * @ingroup usrdevice
 *
 * @param errs_only True to only print errors (not the info)
 * @param shellout True to output to the shell (in addition to the display)
 * @return bool True if successful.
 */
extern const md_info_t* pdusr_info(bool errs_only, bool shellout);

/**
 * @brief Check if the device is empty.
 * @ingroup usrdevice
 *
 * @param shellout True to output to the shell (in addition to the display)
 * @return bool True if the device is empty.
 */
extern bool pdusr_is_empty(bool shellout);

/**
 * @brief Check if a specific sector is empty.
 * @ingroup usrdevice
 *
 *
 * @param sect
 * @param shellout
 * @return bool
 */
extern bool pdusr_is_sect_empty(int sect, bool shellout);

/**
 * @brief Program the device from the file (binary or IntelHEX)
 * @ingroup usrdevice
 *
 * This handles PD_OP_CANCELLED.
 *
 * @param filename The name of the file
 * @param shellout True to output to the shell (in addition to the display)
 * @return pd_op_status_t The status from `pd_prog_file`
 */
extern pd_op_status_t pdusr_prog(const char* filename, bool shellout);

/**
 * @brief Request device power to be turned on (true) or off (false).
 * @ingroup usrdevice
 *
 * Uses `pdo_pwr_request_on` (so follows the same rules) and displays a message
 * if the request was to turn power on and power couldn't be turned on.
 *
 * @param shellout True to output messages to the shell.
 * @param on True to request power on, false to request it off.
 *
 * @return bool
 */
extern bool pdusr_pwr_request_on(bool on, bool shellout);

/**
 * @brief Read the device into a file (as a binary image)
 * @ingroup usrdevice
 *
 * Reads the device into the file. If the file doesn't exist it is created, if
 * it does exist it is truncated before being written.
 *
 * This handles PD_OP_CANCELLED.
 *
 * @param filename The name of the file
 * @param shellout True to output to the shell (in addition to the display)
 * @return pd_op_status_t The status from `pd_prog_file`
 */
extern pd_op_status_t pdusr_read(const char* filename, bool shellout);

/**
 * @brief Verify the device against a file image.
 * @ingroup usrdevice
 *
 * This handles PD_OP_CANCELLED.
 *
 * @param filename The name of the file
 * @param shellout True to output to the shell (in addition to the display)
 * @return pd_op_status_t The status from `pd_verify_file`
 */
extern pd_op_status_t pdusr_verify(const char* filename, bool shellout);


/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 *
 */
extern void pdusr_modinit();

#ifdef __cplusplus
}
#endif
#endif // PDUSR_H_
