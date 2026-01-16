/**
 * APPMENU Menus Module.
 *
 * Application menus (for the display)
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#ifndef APPMENU_H_
#define APPMENU_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "menumgr_t.h"

/**
 * @brief The Main Application Menu
 * @ingroup appmenu
 */
extern const smenu_t app_main_menu;

/**
 * @brief Get the Files Selection Menu
 * @ingroup appmenu
 *
 * Displays files from the root directory of the SD card. The `select_handler` is
 * executed via an Exec Message when a file is selected. The `fselbuf` is filled in
 * with the name of the selected file. The msg data string pointer is also set to
 * point to this.
 *
 * @param select_handler Message handler function, called with the selected file name.
 * @param fselbuf Pointer to storage for the name of the selected file. Must be 256 bytes.
 * @return const dynmenu_t*
 */
extern const dynmenu_t* appmenu_files_select(msg_handler_fn select_handler, char* fselbuf);

/**
 * @brief Initialize the module. Must be called once/only-once before module use.
 * @ingroup appmenu
 *
 */
extern void appmenu_modinit();

#ifdef __cplusplus
}
#endif
#endif // APPMENU_H_
