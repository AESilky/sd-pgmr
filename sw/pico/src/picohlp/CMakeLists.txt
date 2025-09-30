# Library: picoutil - Utilities that help with Pico operations (Library/Interface)
add_library(picoutil INTERFACE)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../
)

target_sources(picoutil INTERFACE
    picoutil.c
)

target_link_libraries(picoutil INTERFACE
    picoutil
    pico_stdlib
)



# Library: rtc_support - Provide either the Pico RTC (if available) or a Virtual RTC (Library/Interface)
add_library(rtc_support INTERFACE)

include_directories(
  ${PICO_SDK_PATH}/src/rp2_common/hardware_rtc/include
)

target_sources(rtc_support INTERFACE
  rtc_support.c
)

target_link_libraries( rtc_support INTERFACE
  pico_stdlib
)

if(PICO_PLATFORM STREQUAL "rp2040")
target_link_libraries(rtc_support INTERFACE
  hardware_rtc
)
else()
add_library(hardware_rtc INTERFACE)

target_sources(hardware_rtc INTERFACE
  vrtc.c
)
endif()

