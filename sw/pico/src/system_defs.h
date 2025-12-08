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

#include "rppins.h"

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
#define SPI_SD_DISP_MISO        GP4
#define SPI_SD_DISP_MOSI        GP3
#define SPI_SD_DISP_SCK         GP2
#define SPI_SD_CS               GP5
#define SPI_DISPLAY_CS          GP6
#define SPI_DISPLAY_CTRL        GP7
#define SPI_SLOW_SPEED         (50 * 1000)      // Very slow speed for init ops
#define SPI_SD_DISP_SPEED      (800 * 1000)     // SPI at 800KHz
#define SPI_CS_ENABLE           0               // Chip Select is active LOW
#define SPI_CS_DISABLE          1               // Chip Select is active LOW

// Certain operations are controlled by a 3:8 decoder (74138)
// The decoder is driven by GPIOs 28(A0), 27(A1), 26(A0)
// The values below are the values used to generate each function.
//
#define OP8_BIT0                GP18            // A0
#define OP8_BIT1                GP19            // A1
#define OP8_BIT2                GP20            // A2
#define OP8_BITS_MASK           0x001C0000      // Mask to set all 3 bits at once: 0000 0000 0001 1100 0000 0000 0000 0000
#define OP8_BITS_SHIFT          18              // Right-Shift needed to move the OP8 bits to the correct GPIO position
// OP8 Operations
#define OP8_NONE                 0              // 0 = No-Op
#define OP8_NOP                  1              // Display Reset
#define OP8_ADDRL_LD             2              // Addr Latch LOW Load
#define OP8_ADDRM_LD             3              // Addr Latch MID Load
#define OP8_ADDRH_LD             4              // Addr Latch HIGH Load
#define OP8_DEV_SEL              5              // Device (FlashROM) Select
#define OP8_NOP2                 6              // No-Operation
#define OP8_NOP3                 7              // No-Operation

// Operations controlled directly by a GPIO
//
#define DISPLAY_RST             GP28            // Hard-reset the display
#define OP_DEVICE_PWR           GP9             // Turns the device power on
#define OP_DATA_RD              GP26            // Enable data from the device to the bus
#define OP_DATA_WR              GP27            // Enable data from the bus to the device
#define OP_DATA_LATCH           GP21            // Latch data to/from the bus/device

// Data Bus
//
#define DATA0                   GP10
#define DATA1                   GP11
#define DATA2                   GP12
#define DATA3                   GP13
#define DATA4                   GP14
#define DATA5                   GP15
#define DATA6                   GP16
#define DATA7                   GP17
#define DATA_BUS_MASK           0x0003FC00      // Mask to set all 8 bits at once: 0000 0000 0000 0011 1111 1100 0000 0000
#define DATA_BUS_SHIFT          10              // Shift to move an 8-bit value up/down to/from the DATA Bus

// PIO Blocks
//
#define PIO_ROTARY_BLOCK        pio1            // PIO Block 1 is used to decode the quadrature signal
#define PIO_ROTARY_SM            0              // State Machine 0 is used for the rotary quad decode
#define PIO_ROTARY_IRQ          PIO1_IRQ_0      // PIO IRQ to use for Rotary reading change
#define PIO_ROTARY_IRQ_IDX       0              // PIO IRQ index for the Rotary reading change

// Rotary Encoder Input
// This is a A/B quadrature encoder that can be decoded using a PIO (must be sequential)
#define ROTARY_A_GPIO           GP0
#define ROTARY_B_GPIO           GP1
#define ROTARY_SW_GPIO          GP8
// Command/Attention switch (separate from rotary encoder)
#define CMD_ATTN_SW_GPIO        GP22

// Switch Control (applies to both Rotary and Command/Attention switches)
//
#define SWITCH_LONGPRESS_MS    450              // 0.45 seconds (450ms) is considered a 'long press'
#define SWITCH_REPEAT_MS       250              // If a switch is long-pressed, repeat it every 1/4 second.
#define SWITCH_PRESSED           0              // Input (digital) is low when the switch is pressed
#define SWITCH_RELEASED          1              // Input (digital) is high when the switch is not pressed


// IRQ Inputs
//
#define IRQ_CMD_ATTN_SW         CMD_ATTN_SW_GPIO
#define IRQ_ROTARY_SW           ROTARY_SW_GPIO

// PWM - Used for a recurring interrupt for scheduled messages, sleep, housekeeping
//    RP2040 has 8 slices, RP2350 has 12. Use the last slice.
//
#if PICO_RP2350
#define CMT_PWM_RECINT_SLICE    11
#else
#define CMT_PWM_RECINT_SLICE     7
#endif

#ifdef __cplusplus
}
#endif
#endif // SYSTEM_DEFS_H_
