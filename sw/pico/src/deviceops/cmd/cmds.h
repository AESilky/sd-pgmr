/**
 * Shell Commands: Device RD/WR command
 *
 * Command to set the device address, read data, and write data.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef CMDS_DEVRDWR_H_
#define XYZ_H_
#ifdef __cplusplus
extern "C" {
#endif


#include "../cmd/cmd_t.h"

extern const cmd_handler_entry_t cmds_devaddr_entry;
extern const cmd_handler_entry_t cmds_devaddr_n_entry;
extern const cmd_handler_entry_t cmds_devpwr_entry;
extern const cmd_handler_entry_t cmds_devrd_entry;
extern const cmd_handler_entry_t cmds_devrd_n_entry;
extern const cmd_handler_entry_t cmds_devwr_entry;
extern const cmd_handler_entry_t cmds_devwr_n_entry;

#ifdef __cplusplus
}
#endif
#endif // CMDS_DEVRDWR_H_
