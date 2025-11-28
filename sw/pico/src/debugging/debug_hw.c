/**
 * Implementation of the Debug Switch Interface.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
*/
#include "board.h" // Declarations for the Debug Switch methods

#define DB_SW_GPIO GP22
#define DB_SW_PRESSED 0

// DEBUG UART (not used)
#define DEBUG_UART_INST         uart0
#define DEBUG_UART_TX_PIN       GP1
#define DEBUG_UART_RX_PIN       GP0
#define DEBUG_UART_BAUDRATE     115200


static bool _nondb_gpio_initialized;

void nondb_gpio_init() {
    if (!_nondb_gpio_initialized) {
        // No special init needed
        _nondb_gpio_initialized = true;
    }
}

void debug_sw_init() {
    gpio_set_function(DB_SW_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(DB_SW_GPIO, GPIO_IN);
    gpio_set_pulls(DB_SW_GPIO, true, false);
}

void debug_uart_init() {
    //
    // Not using the UART for this board
    //
    // gpio_set_function(DEBUG_UART_TX_PIN, UART_FUNCSEL_NUM(DEBUG_UART_INST, DEBUG_UART_TX_PIN));
    // gpio_set_function(DEBUG_UART_RX_PIN, UART_FUNCSEL_NUM(DEBUG_UART_INST, DEBUG_UART_RX_PIN));
    // stdio_uart_init_full(DEBUG_UART_INST, DEBUG_UART_BAUDRATE, DEBUG_UART_TX_PIN, DEBUG_UART_RX_PIN);
}

bool debug_sw_pressed() {
    return (gpio_get(DB_SW_GPIO) == DB_SW_PRESSED);
}
