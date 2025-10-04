/*!
 * \brief Definitions for the board hardware.
 * \ingroup board
 *
 * This contains most of the definitions for the board.
 * Some definitions that are truly local to a module are in the module.
 *
 * Copyright 2023-25 AESilky
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef SYSTEM_DEFS_H_
#define SYSTEM_DEFS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "pico/stdlib.h"
#undef putc     // Undefine so the standard macros will not be used
#undef putchar  // Undefine so the standard macros will not be used

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "pico/multicore.h"

#define ADC_INPUT_0             0               // ADC-0 on GPIO-26 (used with 'adc_select_input')
#define ADC_INPUT_1             1               // ADC-1 on GPIO-27 (used with 'adc_select_input')
#define ADC_INPUT_2             2               // ADC-2 on GPIO-28 (used with 'adc_select_input')
#define ADC_CHIP_TEMP           3               // Internal temperature sensor  (used with 'adc_select_input')

// SPI
//
// Note: Values (Pins) are the GPIO number, not the physical pins on the device.
//

// SPI 0 is used for the SD Card and Display.
//
#define SPI_SD_DISP_DEVICE      spi0            // Hardware SPI to use
#define SPI_SD_DISP_MISO        4               // DP-6
#define SPI_SD_DISP_MOSI        3               // DP-5
#define SPI_SD_DISP_SCK         2               // DP-4
#define SPI_SD_CS               5               // DP-7
#define SPI_DISPLAY_CS          6               // DP-9
#define SPI_SD_DISP_SPEED      (2200 * 1000)    // SPI at 2.2MHz
#define SPI_CS_ENABLE           0               // Chip Select is active LOW
#define SPI_CS_DISABLE          1               // Chip Select is active LOW

// Certain operations are controlled by a 3:8 decoder (74138)
// The decoder is driven by GPIOs 28(A0), 27(A1), 26(A0)
// The values below are the values used to generate each function.
//
#define OP8_BIT0                28              // A0 is GPIO28 (pin 34)
#define OP8_BIT1                27              // A1 is GPIO27 (pin 32)
#define OP8_BIT2                26              // A2 is GPIO26 (pin 31)
#define OP8_BITS_MASK           0x1C000000      // Mask to set all 3 bits at once: 0001 1100 0000 0000 0000 0000 0000 0000
#define OP8_BITS_SHIFT          26              // Right-Shift needed to move the OP8 bits to the correct GPIO position
// NOTE: The board has the upper GPIO going to A0 and the low GPIO going to A2 !!!
//          Therefore, the following address numbers are adjusted so that they can be output directly as 3 bits.
#define OP8_NONE                 0              // 0 = No-Op
#define OP8_ADDRL_LD             4              // 1 (001/100)= Addr Latch LOW Load
#define OP8_ADDRM_LD             2              // 2 (010/010)= Addr Latch MID Load
#define OP8_ADDRH_LD             6              // 3 (011/110)= Addr Latch HIGH Load
#define OP8_DEV_SEL              1              // 4 (100/001)= Device (FlashROM) Select
#define OP8_DISP_RST             5              // 5 (101/101)= Display Reset
#define OP8_DISP_CTRL            3              // 6 (110/011)= Display Control (not Data)
#define OP8_ADDR_CLK             7              // 7 (111/111)= Address Advance Clock

// Operations controlled directly by a GPIO
//
#define OP_DEVICE_PWR            9              // DP-12 Turns the device power on
#define OP_DATA_RD              18              // DP-24 Enable data from the device to the bus
#define OP_DATA_WR              19              // DP-25 Enable data from the bus to the device
#define OP_DATA_LATCH           20              // DP-26 Latch data to/from the bus/device
#define OP_DEVICE_WR            21              // DP-27 Enable the Write on the device

// Data Bus
//
#define DATA0                   10              // DP-14
#define DATA1                   11              // DP-15
#define DATA2                   12              // DP-16
#define DATA3                   13              // DP-17
#define DATA4                   14              // DP-19
#define DATA5                   15              // DP-20
#define DATA6                   16              // DP-21
#define DATA7                   17              // DP-22
#define DATA_BUS_MASK           0x0003FC00      // Mask to set all 8 bits at once: 0000 0000 0000 0011 1111 1100 0000 0000

// UART Functions
//
#define DEBUG_UART           uart0              // UART-0 is used for debug probe serial
#define DEBUG1_TX                0              // DP-1 GPIO0 is used for debug1 TX
#define DEBUG1_RX                1              // DP-2 GPIO1 is used for debug1 RX
#define DEBUG2_TX               12              // DP-14 GPIO12 is used for debug2 TX
#define DEBUG2_RX               13              // DP-15 GPIO13 is used for debug2 RX

// PIO Blocks
//
#define PIO_ROTARY_BLOCK        pio1            // PIO Block 1 is used to decode the quadrature signal
#define PIO_ROTARY_SM            0              // State Machine 0 is used for the rotary quad decode
#define PIO_ROTARY_IRQ          PIO1_IRQ_0      // PIO IRQ to use for Rotary reading change
#define PIO_ROTARY_IRQ_IDX       0              // PIO IRQ index for the Rotary reading change

// Rotary Encoder Input
// This is a A/B quadrature encoder that can be decoded using a PIO (must be sequential)
#define ROTARY_A_GPIO            0              // DP-1
#define ROTARY_B_GPIO            1              // DP-2
#define ROTARY_SW                8              // DP-11
// Command/Attention switch (separate from rotary encoder)
#define CMD_ATTN_SW_GPIO        22              // DP-29

// Switch Control (applies to both Rotary and Command/Attention switches)
//
#define SWITCH_LONGPRESS_MS    800              // 0.8 seconds (800ms) is considered a 'long press'
#define SWITCH_REPEAT_MS       250              // If a switch is long-pressed, repeat it every 1/4 second.
#define SWITCH_PRESSED           0              // Input (digital) is low when the switch is pressed
#define SWITCH_RELEASED          1              // Input (digital) is high when the switch is not pressed


// IRQ Inputs
//
#define IRQ_CMD_ATTN_SW            CMD_ATTN_SW_GPIO
#define IRQ_ROTARY_SW           ROTARY_SW       // DP-11
#define IRQ_ROTARY_TURN         ROTARY_A_GPIO   // DP-1

// PWM - Used for a recurring interrupt for scheduled messages, sleep, housekeeping
//
#define CMT_PWM_RECINT_SLICE    11              // RP2040 has 8 slices, RP2350 has 12.


#ifdef __cplusplus
}
#endif
#endif // SYSTEM_DEFS_H_
