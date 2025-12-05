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

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Invalid Address indicator.
 * @ingroup device
 */
#define PD_INVALID_ADDR (0xFFFFFFFF)

/**
 * @brief Invalid Sector indicator.
 * @ingroup device
 */
#define PD_INVALID_SECT (0xFF)

/**
 * @brief Information about a programmable device.
 * @ingroup device
 */
typedef struct mfg_device_info_s_ {
    uint8_t mfgid;      // Manufacturer ID
    uint8_t devid;      // Device ID
    uint8_t sectcnt;   // Sector Count (ie. 512K device with 64K sectors = 8)
    uint8_t abm;       // Address Bit Max (ie. 16 for a 128K device)
    const char* mfgs;   // Manufacturer Name (string)
    const char* devs;   // Device Name (string)
} md_info_t;

static inline uint32_t pd_sectsize(const md_info_t* info);

/**
 * @brief Get the info for the current programmable device.
 * @ingroup device
 *
 * @return const md_info_t*
 */
extern const md_info_t* pd_info();

/**
 * @brief Check that the programmable device is empty (can be programmed).
 * @ingroup device
 *
 * @return true The device is empty and can be programmed.
 * @return false The device isn't empty, or is unknown - and therefore, can't be programmed.
 */
extern bool pd_is_empty();

/**
 * @brief Check that the programmable device sector is empty (can be programmed).
 * @ingroup device
 *
 * @param sectno The 0-based sector to check
 * @return true The sector is empty and can be programmed.
 * @return false The sector isn't empty, or the device isn't known.
 */
extern bool pd_is_sect_empty(uint8_t sectno);

/**
 * @brief Get the sector for an address within the address.
 * @ingroup device
 *
 * @param info md_info pointer for the device.
 * @return uint8_t The sector containing the address or PD_INVALID_SECT.
 */
static inline uint8_t pd_sect_for_addr(const md_info_t* info, uint32_t addr) {
    uint8_t sect = addr / pd_sectsize(info);
    return (sect < info->sectcnt ? sect : PD_INVALID_SECT);
}

/**
 * @brief Get the size of the sectors for the programmable device from the info.
 * @ingroup device
 *
 * @param info md_info pointer for the device.
 * @return uint32_t The size of the device in bytes.
 */
static inline uint32_t pd_sectsize(const md_info_t* info) {
    return((1 << (info->abm + 1)) / info->sectcnt);
}

/**
 * @brief Get the starting address of a sector (by sector number).
 * @ingroup device
 *
 * @param info md_info pointer for the device.
 * @param sect The sector number (0 - (sectcnt - 1))
 * @return uint32_t Start address of the sector or PD_INVALID_ADDR
 */
extern uint32_t pd_sectstart(const md_info_t* info, uint8_t sect);

/**
 * @brief Get the size of the programmable device from the info.
 * @ingroup device
 *
 * @param info md_info pointer for the device.
 * @return uint32_t The size of the device in bytes.
 */
static inline uint32_t pd_size(const md_info_t* info) {
    return(1 << (info->abm + 1));
}



/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 * @ingroup device
 *
 */
extern void pd_minit();

#ifdef __cplusplus
}
#endif
#endif // PROG_DEVICE_H_
