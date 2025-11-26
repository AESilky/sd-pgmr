/**
 * Board Initialization and General Functions.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 *
 * This sets up the Pico.
 * It:
 * 1. Configures the GPIO Pins for the proper IN/OUT, pull-ups, etc.
 * 2. Calls the init routines for Config, UI (Display, Touch, rotary)
 *
 * It provides some utility methods to:
 * 1. Turn the On-Board LED ON/OFF
 * 2. Flash the On-Board LED a number of times
 * 3. Error, Warn, Info, Debug 'printf' routines
 *
*/
#include "system_defs.h"
#include "rtc_support.h"

#include "board.h"

#include "debug_support.h"
#include "util.h"

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "pico/mutex.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/uart.h"

#include <stdio.h>
#include <stdlib.h>

// ////////////////////////
// /// Board Level Data ///
// ////////////////////////
//

volatile bool _diagout_disabled;

char shared_print_buf[SHARED_PRINT_BUF_SIZE];


/** @brief `bop_mutex` is used for performing Board Op control signal changes. */
auto_init_mutex(bop_mutex);

bool _databus_is_out() {
    return gpio_get_dir(DATA0); // We check DATA-0. All DATA bits direction are set as one.
}

/**
 * @brief Initialize the board
 *
 * This sets up the GPIO for the proper direction (IN/OUT), pull-ups, etc.
 * This calls the init for each of the devices/subsystems.
 * If all is okay, it returns 0, else non-zero.
 *
 * Although each subsystem could (some might argue should) configure their own Pico
 * pins, having everything here makes the overall system easier to understand
 * and helps assure that there are no conflicts.
*/
int board_init() {
    int retval = 0;

    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_put(LED_PIN, 1);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // SPI 0 Pins for MircoSD Card and Display
    gpio_set_function(SPI_SD_DISP_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SD_DISP_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SD_DISP_MISO, GPIO_FUNC_SPI);
    // SPI 0 Signal drive strengths
    gpio_set_drive_strength(SPI_SD_DISP_SCK, GPIO_DRIVE_STRENGTH_4MA);     // Two devices connected
    gpio_set_drive_strength(SPI_SD_DISP_SCK, GPIO_DRIVE_STRENGTH_4MA);    // Two devices connected
    // SPI 0 Data In Pull-Up
    gpio_pull_up(SPI_SD_DISP_MISO);
    // SPI 0 initialization for the MicroSD Card and Display.
    spi_init(SPI_SD_DISP_DEVICE, SPI_SD_DISP_SPEED);


    // GPIO Outputs (other than SPI, I2C, UART, and chip-selects

    //  Display
    gpio_set_function(SPI_DISPLAY_CS, GPIO_FUNC_SIO);
    gpio_put(SPI_DISPLAY_CS, 1);
    gpio_set_dir(SPI_DISPLAY_CS, GPIO_OUT);
    gpio_set_drive_strength(SPI_DISPLAY_CS, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(SPI_DISPLAY_CTRL, GPIO_FUNC_SIO);
    gpio_put(SPI_DISPLAY_CTRL, 1);
    gpio_set_dir(SPI_DISPLAY_CTRL, GPIO_OUT);
    gpio_set_drive_strength(SPI_DISPLAY_CTRL, GPIO_DRIVE_STRENGTH_2MA);

    // Operation Address Bits
    gpio_set_function(OP8_BIT0, GPIO_FUNC_SIO);
    gpio_put(OP8_BIT0, 0);
    gpio_set_dir(OP8_BIT0, GPIO_OUT);
    gpio_set_drive_strength(OP8_BIT0, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(OP8_BIT1, GPIO_FUNC_SIO);
    gpio_put(OP8_BIT1, 0);
    gpio_set_dir(OP8_BIT1, GPIO_OUT);
    gpio_set_drive_strength(OP8_BIT1, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(OP8_BIT2, GPIO_FUNC_SIO);
    gpio_put(OP8_BIT2, 0);
    gpio_set_dir(OP8_BIT2, GPIO_OUT);
    gpio_set_drive_strength(OP8_BIT2, GPIO_DRIVE_STRENGTH_2MA);
    // Operation Control
    gpio_set_function(OP_DEVICE_PWR, GPIO_FUNC_SIO);
    gpio_put(OP_DEVICE_PWR, 0);
    gpio_set_dir(OP_DEVICE_PWR, GPIO_OUT);
    gpio_set_drive_strength(OP_DEVICE_PWR, GPIO_DRIVE_STRENGTH_12MA);
    //
    // DP_DATA_RD is initialized in the debug init (debug_hw.c)
    //
    // gpio_set_function(OP_DATA_RD, GPIO_FUNC_SIO);
    // gpio_put(OP_DATA_RD, 1);
    // gpio_set_dir(OP_DATA_RD, GPIO_OUT);
    // gpio_set_drive_strength(OP_DATA_RD, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(OP_DATA_WR, GPIO_FUNC_SIO);
    gpio_set_dir(OP_DATA_WR, GPIO_OUT);
    gpio_put(OP_DATA_WR, 1);
    gpio_set_drive_strength(OP_DATA_WR, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(OP_DATA_LATCH, GPIO_FUNC_SIO);
    gpio_put(OP_DATA_LATCH, 0);
    gpio_set_dir(OP_DATA_LATCH, GPIO_OUT);
    gpio_set_drive_strength(OP_DATA_LATCH, GPIO_DRIVE_STRENGTH_2MA);

    // GPIO Inputs

    //  Data Bus (Initially set to input)
    gpio_set_function(DATA0, GPIO_FUNC_SIO);
    gpio_set_dir(DATA0, GPIO_IN);
    gpio_set_drive_strength(DATA0, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(DATA1, GPIO_FUNC_SIO);
    gpio_set_dir(DATA1, GPIO_IN);
    gpio_set_drive_strength(DATA1, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(DATA2, GPIO_FUNC_SIO);
    gpio_set_dir(DATA2, GPIO_IN);
    gpio_set_drive_strength(DATA2, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(DATA3, GPIO_FUNC_SIO);
    gpio_set_dir(DATA3, GPIO_IN);
    gpio_set_drive_strength(DATA3, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(DATA4, GPIO_FUNC_SIO);
    gpio_set_dir(DATA4, GPIO_IN);
    gpio_set_drive_strength(DATA4, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(DATA5, GPIO_FUNC_SIO);
    gpio_set_dir(DATA5, GPIO_IN);
    gpio_set_drive_strength(DATA5, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(DATA6, GPIO_FUNC_SIO);
    gpio_set_dir(DATA6, GPIO_IN);
    gpio_set_drive_strength(DATA6, GPIO_DRIVE_STRENGTH_2MA);
    gpio_set_function(DATA7, GPIO_FUNC_SIO);
    gpio_set_dir(DATA7, GPIO_IN);
    gpio_set_drive_strength(DATA7, GPIO_DRIVE_STRENGTH_2MA);

    //    Rotary Encoder Input
    gpio_set_function(ROTARY_A_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(ROTARY_A_GPIO, GPIO_IN);
    gpio_set_pulls(ROTARY_A_GPIO, true, false);
    gpio_set_function(ROTARY_B_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(ROTARY_B_GPIO, GPIO_IN);
    gpio_set_pulls(ROTARY_B_GPIO, true, false);
    //    Rotary Encoder Switch Input
    gpio_set_function(ROTARY_SW_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(ROTARY_SW_GPIO, GPIO_IN);
    gpio_set_pulls(ROTARY_SW_GPIO, true, false);
    //    Command Attention (CMDATTN) Switch Input
    gpio_set_function(CMD_ATTN_SW_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(CMD_ATTN_SW_GPIO, GPIO_IN);
    gpio_set_pulls(CMD_ATTN_SW_GPIO, true, false);

    //
    // Module initialization that is needed for other modules to initialize.
    //


    // Initialize the board RTC (or Virtual RTC).
    // Start on Sunday the 1st of January 2023 00:00:01
    datetime_t t = {
            .year = 2023,
            .month = 01,
            .day = 01,
            .dotw = 0, // 0 is Sunday
            .hour = 00,
            .min = 00,
            .sec = 01
    };
    rtc_init();
    rtc_set_datetime(&t);
    // clk_sys is >2000x faster than clk_rtc, so datetime is not updated immediately when rtc_set_datetime() is called.
    // tbe delay is up to 3 RTC clock cycles (which is 64us with the default clock settings)
    sleep_us(100);

    // The PWM is used for a recurring interrupt in CMT. It will initialize it.

    return(retval);
}

void board_op(boptkn_t boptkn, boardop_t bdop) {
    if (boptkn != &bop_mutex) {
        board_panic("board_op: Called with incorrect token: %p  Should be: %p\n", boptkn, &bop_mutex);
    }
    uint32_t op_bits = (bdop << OP8_BITS_SHIFT);
    gpio_put_masked(OP8_BITS_MASK, op_bits);
}

boptkn_t board_op_start() {
    uint32_t owner;
    boptkn_t token = NULL;
    bool success = mutex_try_enter(&bop_mutex, &owner);
    if (!success) {
        debug_printf("board_op_start: Mutex already owned by: %ud\n", owner);
    }
    else {
        token = &bop_mutex;
    }
    return token;
}

void board_op_end(boptkn_t boptkn) {
    if (boptkn != &bop_mutex) {
        board_panic("board_op_end: Called with incorrect token: %p  Should be: %p\n", boptkn, &bop_mutex);
    }
    mutex_exit(&bop_mutex);
}

bool cmdattn_switch_pressed() {
    return (gpio_get(CMD_ATTN_SW_GPIO) == SWITCH_PRESSED);
}

void diagout_enable(bool enable) {
    _diagout_disabled = !enable;
}

bool diagout_is_enabled() {
    return !_diagout_disabled;
}

bool rotary_switch_pressed() {
    return (gpio_get(ROTARY_SW_GPIO) == SWITCH_PRESSED);
}

extern uint8_t pdatabus_rd() {
    if (_databus_is_out()) {
        pdatabus_set_in();
    }
    uint32_t rawvalue = gpio_get_all();
    uint8_t value = (rawvalue & DATA_BUS_MASK) >> DATA_BUS_SHIFT;

    return value;
}

void pdatabus_wr(uint8_t data) {
    if (!_databus_is_out()) {
        // Set the DATA Bus to outbound
        gpio_set_dir_out_masked(DATA_BUS_MASK);
    }
    uint32_t bdval = data << DATA_BUS_SHIFT;
    gpio_put_masked(DATA_BUS_MASK, bdval);
}



void error_printf(const char* format, ...) {
    if (!_diagout_disabled) {
        int index = 0;
        va_list xArgs;
        va_start(xArgs, format);
        index += vsnprintf(&shared_print_buf[index], SHARED_PRINT_BUF_SIZE - index, format, xArgs);
        va_end(xArgs);
        printf("%s", shared_print_buf);
        stdio_flush();
    }
}

void info_printf(const char* format, ...) {
    if (!_diagout_disabled) {
        int index = 0;
        va_list xArgs;
        va_start(xArgs, format);
        index += vsnprintf(&shared_print_buf[index], SHARED_PRINT_BUF_SIZE - index, format, xArgs);
        va_end(xArgs);
        printf("%s", shared_print_buf);
        stdio_flush();
    }
}

void warn_printf(const char* format, ...) {
    if (!_diagout_disabled) {
        int index = 0;
        va_list xArgs;
        va_start(xArgs, format);
        index += vsnprintf(&shared_print_buf[index], SHARED_PRINT_BUF_SIZE - index, format, xArgs);
        va_end(xArgs);
        printf("%s", shared_print_buf);
        stdio_flush();
    }
}



void board_panic(const char* fmt, ...) {
    // Turn the LED on before the panic
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    va_list xArgs;
    va_start(xArgs, fmt);
    error_printf(fmt, xArgs);
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    panic(fmt, xArgs);
    va_end(xArgs);
}

