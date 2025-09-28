# SilkyDESIGN Flash Programmer primary project build (CMake) file.
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_PLATFORM "rp2350-arm-s")
set(PICO_BOARD "pico2")
set(PICO_USE_MALLOC_MUTEX 1)
set(PICO_QUEUE_MAX_LEVEL 1)
# Use fully-deoptimized debug build for true single-step and data viewing. Set to '0' for some optimizations
set(PICO_DEOPTIMIZED_DEBUG 1)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.2.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# For debug output from USB (pass -DPICO_STDIO_USB=1) this ensures we don't lose any debug output while USB is set up
if (NOT DEFINED PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS)
    set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS 3000)
endif()

project(SD_Programmer C CXX ASM)

add_compile_options(
  -O0                       # No optimizations for now, for debugging
  -Wall
  -Wno-format               # int != int32_t as far as the compiler is concerned
  -Wno-unused-function
  -Wno-maybe-uninitialized
  #-H                       # list include header paths
)
# ZZZ Figure out how to set `PICO_DEBUG_MALLOC` only if 'debug build'
add_compile_definitions(
  PICO_MALLOC_PANIC
  PICO_USE_MALLOC_MUTEX
  PICO_QUEUE_MAX_LEVEL
  PICO_USE_STACK_GUARDS
  PICO_STACK_SIZE=2048
  PICO_CORE1_STACK_SIZE=2048
  # PICO_DEBUG_MALLOC
  PICO_MAX_SHARED_IRQ_HANDLERS=6u
  PICO_STDIO_USB_CONNECTION_WITHOUT_DTR
  PICO_STDIO_USB_SUPPORT_CHARS_AVAILABLE_CALLBACK
)
# Initialize the SDK
pico_sdk_init()

#
# This binary can be built 3 ways. Because there aren't enough GPIOs to dedicate
# a debugging serial port to a set of pins, this can be built as:
#   1. No debug serial port
#   2. Debug serial port on GPIO-0 & GPIO-1 (shared with the Rotary Encoder inputs)
#   3. Debug serial port on GPIO-12 & GPIO-13 (shared with the Data Bus (D2 & D3))
#
# The following defines are used for the 3 targets.
#
set(VERSION "1.0")
set(NAME_BASE "SD_Programmer")
# Add the required include file paths to the build
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pwm/include
  ${PICO_SDK_PATH}/src/rp2_common/pico_multicore/include
)

# Our Modules
set(OUR_MODULES
  cmt
  display
  hid
  hwrt
  rotary_encoder
  util
)
#
add_subdirectory(cmt)
add_subdirectory(display)
add_subdirectory(hid)
add_subdirectory(hwrt)
add_subdirectory(rotary_encoder)
add_subdirectory(util)
#
# External libraries
add_subdirectory(lib)
#
# Pico Libraries/Modules
set(PICO_LIBS
  hardware_adc
  hardware_clocks
  hardware_dma
  hardware_exception
  hardware_pio
  hardware_spi
  hardware_timer
  pico_multicore
  pico_stdlib
)
#
# Top-level sources
#
set(TL_SRC
main.c
board.c
debug_support.c
multicore.c
picoutil.c
)

#
# Add executable (output) for No Debug Serial Port
#
add_executable(sdpgmr
  ${TL_SRC}
)
pico_set_program_name(sdpgmr ${NAME_BASE})
pico_set_program_version(sdpgmr ${VERSION})
# Done use any stdio.
pico_enable_stdio_uart(sdpgmr 0)
pico_enable_stdio_usb(sdpgmr 0)
target_compile_definitions(sdpgmr PRIVATE
	DEBUG_SERIAL=0
)
target_include_directories(sdpgmr PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(sdpgmr
  ${OUR_MODULES}
  ${PICO_LIBS}
 )

#
# Add executable (output) for Debug Serial Port Pins-1
#
add_executable(sdpgmr_db1
  ${TL_SRC}
)
pico_set_program_name(sdpgmr_db1 "${NAME_BASE}_DB1")
pico_set_program_version(sdpgmr_db1 ${VERSION})
# Done use any stdio.
pico_enable_stdio_uart(sdpgmr_db1 1)
pico_enable_stdio_usb(sdpgmr_db1 0)
target_compile_definitions(sdpgmr_db1 PRIVATE
	DEBUG_SERIAL=1
  PICO_DEFAULT_UART_TX_PIN=0
  PICO_DEFAULT_UART_RX_PIN=1
)
target_include_directories(sdpgmr_db1 PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(sdpgmr_db1
  ${OUR_MODULES}
  ${PICO_LIBS}
 )

#
# Add executable (output) for Debug Serial Port Pins-2
#
add_executable(sdpgmr_db2
  ${TL_SRC}
)
pico_set_program_name(sdpgmr_db2 "${NAME_BASE}_DB1")
pico_set_program_version(sdpgmr_db2 ${VERSION})
# Done use any stdio.
pico_enable_stdio_uart(sdpgmr_db2 1)
pico_enable_stdio_usb(sdpgmr_db2 0)
target_compile_definitions(sdpgmr_db2 PRIVATE
	DEBUG_SERIAL=2
  PICO_DEFAULT_UART_TX_PIN=12
  PICO_DEFAULT_UART_RX_PIN=13
)
target_include_directories(sdpgmr_db2 PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(sdpgmr_db2
  ${OUR_MODULES}
  ${PICO_LIBS}
 )

