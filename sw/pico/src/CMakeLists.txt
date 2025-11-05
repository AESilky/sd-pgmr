# SilkyDESIGN Flash Programmer primary project build (CMake) file.
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_PLATFORM "rp2040-arm-s")
set(PICO_BOARD "pico")
set(PICO_USE_MALLOC_MUTEX 1)
set(PICO_QUEUE_MAX_LEVEL 1)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Building in Debug mode. (EAS)")
  # Use fully-deoptimized debug build for true single-step and data viewing.
  set(PICO_DEOPTIMIZED_DEBUG 1)
  # Add debug-specific compiler flags and definitions
  add_compile_options(
    -O0     # No optimizations
  )
  add_compile_definitions(DEBUG_MODE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Building in Release mode. (EAS)")
  set(PICO_DEOPTIMIZED_DEBUG 0)
  add_compile_options(
    -O2     # Optimize
  )
  add_compile_definitions(NDEBUG)
else()
    message(STATUS "Building with unknown configuration: ${CMAKE_BUILD_TYPE}")
endif()


# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.2.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# For debug output from USB (pass -DPICO_STDIO_USB=1) this ensures we don't lose any debug output while USB is set up
if (NOT DEFINED PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS)
    set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS 2500)
endif()
if (NOT DEFINED PICO_STDIO_USB_POST_CONNECT_WAIT_DELAY_MS)
    set(PICO_STDIO_USB_POST_CONNECT_WAIT_DELAY_MS 15)
endif()

# Don't enable the UART STDIO globally. Enabling is handled in 'debug_support' in the code.
set(PICO_STDIO_UART 0)
# Don't enable the USB STDIO globally. Enabling is handled in 'debug_support' in the code.
set(PICO_STDIO_USB 0)


project(SD_Programmer C CXX ASM)


add_compile_options(
  -Wall
  -Wno-format               # int != int32_t as far as the compiler is concerned
  -Wno-unused-function
  -Wno-maybe-uninitialized
  #-H                       # list include header paths
)
add_compile_definitions(
  PICO_INCLUDE_RTC_DATETIME=1
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

set(VERSION "1.0")
set(NAME_BASE "SD_Programmer")
# Add the required include file paths to the build
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${PICO_SDK_PATH}/src/common/pico_base_headers/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_adc/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_pwm/include
  ${PICO_SDK_PATH}/src/rp2_common/hardware_rtc/include
  ${PICO_SDK_PATH}/src/rp2_common/pico_multicore/include
  ${PICO_SDK_PATH}/src/rp2_common/pico_stdio_uart/include
  ${PICO_SDK_PATH}/src/rp2_common/pico_stdio_usb/include
)

# Our Modules
set(OUR_MODULES
  cmt
  display
  app
  hwrt
  picoutil
  prog_device
  rotary_encoder
)
#
add_subdirectory(cmt)
add_subdirectory(deviceops)
add_subdirectory(display)
add_subdirectory(dskops)
add_subdirectory(app)
add_subdirectory(hwrt)
add_subdirectory(lib)
add_subdirectory(picohlp)
add_subdirectory(rotary_encoder)
add_subdirectory(shell)
#
# External libraries
#add_subdirectory(lib)
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
  pico_stdio_uart
  pico_stdio_usb
)
#
# Top-level sources
#
set(TL_SRC
main.c
board.c
debug_hw.c
debug_support.c
multicore.c
util.c
)

#
# Add executable (output) for Debug Mode disabled by default
#
add_executable(sdpgmr
  ${TL_SRC}
)
pico_set_program_name(sdpgmr ${NAME_BASE})
pico_set_program_version(sdpgmr ${VERSION})
target_compile_definitions(sdpgmr PRIVATE
	DEBUG_MODE=0
)
target_include_directories(sdpgmr PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(sdpgmr
  ${OUR_MODULES}
  ${PICO_LIBS}
)
#pico_set_linker_script(sdpgmr ${CMAKE_SOURCE_DIR}/memmap_custom.ld)
pico_add_extra_outputs(sdpgmr)


#
# Add executable (output) for Debug Mode enabled by default
#
add_executable(sdpgmr_db
  ${TL_SRC}
)
pico_set_program_name(sdpgmr_db "${NAME_BASE}_DB")
pico_set_program_version(sdpgmr_db ${VERSION})
target_compile_definitions(sdpgmr_db PRIVATE
	DEBUG_MODE=1
)
target_include_directories(sdpgmr_db PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(sdpgmr_db
  ${OUR_MODULES}
  ${PICO_LIBS}
)
#pico_set_linker_script(sdpgmr_db ${CMAKE_SOURCE_DIR}/memmap_custom.ld)
pico_add_extra_outputs(sdpgmr_db)
