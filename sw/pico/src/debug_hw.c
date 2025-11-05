/**
 * Implementation of the Debug Switch Interface.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "board.h" // Declarations for the Debug Switch methods

#define DB_SW_GPIO 22
#define DB_SW_PRESSED 0

// DEBUG UART (used for initial startup. Then DEBUG is switched to the USB)
#define DEBUG_UART_INST         uart0
#define DEBUG_UART_TX_PIN       DATA2
#define DEBUG_UART_RX_PIN       DATA3
#define DEBUG_UART_BAUDRATE     115200


static bool _nondb_gpio_initialized;

void nondb_gpio_init() {
    if (!_nondb_gpio_initialized) {
        gpio_set_function(DATA2, GPIO_FUNC_SIO);
        gpio_set_dir(DATA2, GPIO_IN);
        gpio_set_drive_strength(DATA2, GPIO_DRIVE_STRENGTH_2MA);
        gpio_set_function(DATA3, GPIO_FUNC_SIO);
        gpio_set_dir(DATA3, GPIO_IN);
        gpio_set_drive_strength(DATA3, GPIO_DRIVE_STRENGTH_2MA);
        _nondb_gpio_initialized = true;
    }
}

void debug_sw_init() {
    gpio_set_function(DB_SW_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(DB_SW_GPIO, GPIO_IN);
    gpio_set_pulls(DB_SW_GPIO, true, false);
}

void debug_uart_init() {
    // PD DATA 2 and 3 are shared with the DEBUG UART
    //
    // Make sure the data read latch isn't driving the PD lines
    gpio_set_function(OP_DATA_RD, GPIO_FUNC_SIO);
    gpio_put(OP_DATA_RD, 1);
    gpio_set_dir(OP_DATA_RD, GPIO_OUT);
    gpio_set_drive_strength(OP_DATA_RD, GPIO_DRIVE_STRENGTH_2MA);
    //
    // Init DATA2 and DATA3 for the UART
    gpio_set_function(DEBUG_UART_TX_PIN, UART_FUNCSEL_NUM(DEBUG_UART_INST, DEBUG_UART_TX_PIN));
    gpio_set_function(DEBUG_UART_RX_PIN, UART_FUNCSEL_NUM(DEBUG_UART_INST, DEBUG_UART_RX_PIN));
    stdio_uart_init_full(DEBUG_UART_INST, DEBUG_UART_BAUDRATE, DEBUG_UART_TX_PIN, DEBUG_UART_RX_PIN);
}

bool debug_sw_pressed() {
    return (gpio_get(DB_SW_GPIO) == DB_SW_PRESSED);
}
