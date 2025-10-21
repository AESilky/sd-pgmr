/**
 * Debugging flags and utilities.
 *
 * Copyright 2023-25 AESilky
 * SPDX-License-Identifier: MIT License
 */
#include "debug_support.h"

#include "board.h"

#include "cmt/cmt.h"
#include "util.h"

#include "tusb.h"
#include "pico/printf.h"
#include "pico/stdio_uart.h"
#include "pico/stdio_usb.h"

#include <stdlib.h>

#ifdef uart_default
  #undef uart_default
#endif

volatile uint16_t debugging_flags = 0;
static bool _debug_mode_enabled = false;


void debug_init(debug_init_mode_t mode) {
    switch (mode) {
    case DIM_BOOT:
        // Init an input switch
        // Init UART0
        // Connect STDIO to UART0
        // Read switch to set debug enabled flag
        debug_sw_init();
        debug_uart_init();
        sleep_ms(80); // Ok to `sleep` as msg system not started
        // Check the switch
        bool pressed = debug_sw_pressed();
#if (DEBUG_MODE != 0)
// In debug build, set debug flag unless switch pressed
        debug_mode_enable(!pressed);
#else
// In release build, set debug flag if switch pressed
        debug_mode_enable(pressed);
#endif
        // initialize TinyUSB so that it's ready later.
        tusb_init();

        debug_printf("Debug initialized\n");
        break;
    case DIM_STDIO_TO_USB:
        stdio_flush();
        sleep_ms(8);
        // Switch STDIO from the UART to the USB
        stdio_set_driver_enabled(&stdio_uart, false);
        sleep_ms(2); // Short sleep ok
        stdio_usb_init();
        nondb_gpio_init(); // Init the GPIO that was skipped to allow UART
        break;
    case DIM_STDIO_TO_USB_DIUART:
        stdio_flush();
        sleep_ms(8);
        // Switch STDIO from the UART to the USB and deinit the UART
        stdio_uart_deinit();
        sleep_ms(2);
        stdio_usb_init();
        nondb_gpio_init(); // Init the GPIO that was skipped to allow UART
        break;
    case DIM_REMOVE_STDIO:
        // Remove STDIO from both the UART and the USB
        stdio_set_driver_enabled(&stdio_uart, false);
        stdio_set_driver_enabled(&stdio_usb, false);
        nondb_gpio_init(); // Init the GPIO that was skipped to allow UART
        break;
    }
}


bool debug_mode_enabled() {
    return _debug_mode_enabled;
}

bool debug_mode_enable(bool on) {
    bool temp = _debug_mode_enabled;
    _debug_mode_enabled = on;
    if (on != temp && cmt_message_loops_running()) {
        cmt_msg_t msg;
        cmt_msg_init(&msg, MSG_DEBUG_CHANGED);
        msg.data.debug = _debug_mode_enabled;
        postHWRTMsgDiscardable(&msg);
        postAPPMsgDiscardable(&msg);
    }
    return (temp != on);
}



void debug_printf(const char* format, ...) {
    if (debug_mode_enabled() && diagout_is_enabled()) {
        int index = 0;
        va_list xArgs;
        va_start(xArgs, format);
        index += vsnprintf(&shared_print_buf[index], SHARED_PRINT_BUF_SIZE - index, format, xArgs);
        va_end(xArgs);
        printf("%s", shared_print_buf);
        stdio_flush();
    }
}

/**
 * This is called from the HardFaultHandler with a pointer the Fault stack
 * as the parameter. We can then read the values from the stack and place them
 * into local variables for ease of reading.
 * We then read the various Fault Status and Address Registers to help decode
 * cause of the fault.
 * The function ends with a BKPT instruction to force control back into the debugger
 */
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
void McuHardFault_HandlerC(uint32_t* hardfault_args) {
  /*lint -save  -e550 Symbol not accessed. */
    static volatile unsigned long stacked_r0;
    static volatile unsigned long stacked_r1;
    static volatile unsigned long stacked_r2;
    static volatile unsigned long stacked_r3;
    static volatile unsigned long stacked_r12;
    static volatile unsigned long stacked_lr;
    static volatile unsigned long stacked_pc;
    static volatile unsigned long stacked_psr;
    static volatile unsigned long _CFSR;
    static volatile unsigned long _HFSR;
    static volatile unsigned long _DFSR;
    static volatile unsigned long _AFSR;
    static volatile unsigned long _BFAR;
    static volatile unsigned long _MMAR;
    stacked_r0 = ((unsigned long)hardfault_args[0]);          /* http://www.asciiworld.com/-Smiley,20-.html                                   */
    stacked_r1 = ((unsigned long)hardfault_args[1]);          /*                         oooo$$$$$$$$$$$$oooo                                 */
    stacked_r2 = ((unsigned long)hardfault_args[2]);          /*                      oo$$$$$$$$$$$$$$$$$$$$$$$$o                             */
    stacked_r3 = ((unsigned long)hardfault_args[3]);          /*                    oo$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$o         o$   $$ o$      */
    stacked_r12 = ((unsigned long)hardfault_args[4]);         /*    o $ oo        o$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$o       $$ $$ $$o$     */
    stacked_lr = ((unsigned long)hardfault_args[5]);          /* oo $ $ "$      o$$$$$$$$$    $$$$$$$$$$$$$    $$$$$$$$$o       $$$o$$o$      */
    stacked_pc = ((unsigned long)hardfault_args[6]);          /* "$$$$$$o$     o$$$$$$$$$      $$$$$$$$$$$      $$$$$$$$$$o    $$$$$$$$       */
    stacked_psr = ((unsigned long)hardfault_args[7]);         /*   $$$$$$$    $$$$$$$$$$$      $$$$$$$$$$$      $$$$$$$$$$$$$$$$$$$$$$$       */
                                                              /*   $$$$$$$$$$$$$$$$$$$$$$$    $$$$$$$$$$$$$    $$$$$$$$$$$$$$  """$$$         */
    /* Configurable Fault Status Register */                  /*    "$$$""""$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$     "$$$        */
    /* Consists of MMSR, BFSR and UFSR */                     /*     $$$   o$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$     "$$$o      */
    _CFSR = (*((volatile unsigned long*)(0xE000ED28)));       /*    o$$"   $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$       $$$o     */
                                                              /*    $$$    $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" "$$$$$$ooooo$$$$o   */
    /* Hard Fault Status Register */                          /*   o$$$oooo$$$$$  $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$   o$$$$$$$$$$$$$$$$$  */
    _HFSR = (*((volatile unsigned long*)(0xE000ED2C)));       /*   $$$$$$$$"$$$$   $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$     $$$$""""""""        */
                                                              /*  """"       $$$$    "$$$$$$$$$$$$$$$$$$$$$$$$$$$$"      o$$$                 */
    /* Debug Fault Status Register */                         /*             "$$$o     """$$$$$$$$$$$$$$$$$$"$$"         $$$                  */
    _DFSR = (*((volatile unsigned long*)(0xE000ED30)));       /*               $$$o          "$$""$$$$$$""""           o$$$                   */
                                                              /*                $$$$o                                o$$$"                    */
    /* Auxiliary Fault Status Register */                     /*                 "$$$$o      o$$$$$$o"$$$$o        o$$$$                      */
    _AFSR = (*((volatile unsigned long*)(0xE000ED3C)));       /*                   "$$$$$oo     ""$$$$o$$$$$o   o$$$$""                       */
                                                              /*                      ""$$$$$oooo  "$$$o$$$$$$$$$"""                          */
                                                              /*                         ""$$$$$$$oo $$$$$$$$$$                               */
    /* Read the Fault Address Registers. */                   /*                                 """"$$$$$$$$$$$                              */
    /* These may not contain valid values. */                 /*                                     $$$$$$$$$$$$                             */
    /* Check BFARVALID/MMARVALID to see */                    /*                                      $$$$$$$$$$"                             */
    /* if they are valid values */                            /*                                       "$$$""                                 */
    /* MemManage Fault Address Register */
    _MMAR = (*((volatile unsigned long*)(0xE000ED34)));
    /* Bus Fault Address Register */
    _BFAR = (*((volatile unsigned long*)(0xE000ED38)));

#if 0 /* experimental, seems not to work properly with GDB in KDS V3.2.0 */
#ifdef __GNUC__ /* might improve stack, see https://www.element14.com/community/message/199113/l/gdb-assisted-debugging-of-hard-faults#199113 */
    __asm volatile (
    "tst lr,#4     \n" /* check which stack pointer we are using */
        "ite eq        \n"
        "mrseq r0, msp \n" /* use MSP */
        "mrsne r0, psp \n" /* use PSP */
        "mov sp, r0    \n" /* set stack pointer so GDB shows proper stack frame */
        );
#endif
#endif
    __asm("BKPT #0\n"); /* cause the debugger to stop */
    /*lint -restore */
}

/*
** ===================================================================
**     Method      :  HardFaultHandler (component HardFault)
**
**     Description :
**         Hard Fault Handler
**     Parameters  : None
**     Returns     : Nothing
** ===================================================================
*/
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
__attribute__((naked))
#if McuLib_CONFIG_SDK_VERSION_USED==McuLib_CONFIG_SDK_RPI_PICO
void isr_hardfault(void)
#elif McuLib_CONFIG_SDK_VERSION_USED != McuLib_CONFIG_SDK_PROCESSOR_EXPERT
void HardFault_Handler(void)
#else
void McuHardFault_HardFaultHandler(void)
#endif
{
    __asm volatile (
    ".syntax unified              \n"  /* needed for the 'adds r1,#2' below */
        " movs r0,#4                  \n"  /* load bit mask into R0 */
        " mov r1, lr                  \n"  /* load link register into R1 */
        " tst r0, r1                  \n"  /* compare with bitmask */
        " beq _MSP                    \n"  /* if bitmask is set: stack pointer is in PSP. Otherwise in MSP */
        " mrs r0, psp                 \n"  /* otherwise: stack pointer is in PSP */
        " b _GetPC                    \n"  /* go to part which loads the PC */
        "_MSP:                          \n"  /* stack pointer is in MSP register */
        " mrs r0, msp                 \n"  /* load stack pointer into R0 */
        "_GetPC:                        \n"  /* find out where the hard fault happened */
        " ldr r1,[r0,#24]             \n"  /* load program counter into R1. R1 contains address of the next instruction where the hard fault happened */
#if McuHardFault_CONFIG_SETTING_SEMIHOSTING
  /* The following code checks if the hard fault is caused by a semihosting BKPT instruction which is "BKPT 0xAB" (opcode: 0xBEAB)
     The idea is taken from the MCUXpresso IDE/SDK code, so credits and kudos to the MCUXpresso IDE team! */
        " ldrh r2,[r1]                \n"  /* load opcode causing the fault */
        " ldr r3,=0xBEAB              \n"  /* load constant 0xBEAB (BKPT 0xAB) into R3" */
        " cmp r2,r3                   \n"  /* is it the BKPT 0xAB? */
        " beq _SemihostReturn         \n"  /* if yes, return from semihosting */
        " b McuHardFault_HandlerC   \n"  /* if no, dump the register values and halt the system */
        "_SemihostReturn:               \n"  /* returning from semihosting fault */
        " adds r1,#2                  \n"  /* r1 points to the semihosting BKPT instruction. Adjust the PC to skip it (2 bytes) */
        " str r1,[r0,#24]             \n"  /* store back the adjusted PC value to the interrupt stack frame */
        " movs r1,#32                 \n"  /* need to pass back a return value to emulate a successful semihosting operation. 32 is an arbitrary value */
        " str r1,[r0,#0]              \n"  /* store the return value on the stack frame */
        " bx lr                       \n"  /* return from the exception handler back to the application */
#else
        " b McuHardFault_HandlerC   \n"  /* decode more information. R0 contains pointer to stack frame */
#endif
        );
}

