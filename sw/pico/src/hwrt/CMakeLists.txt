# Library: Hardware Runtime for board address 0 (hwrt0) (obj only)
add_library(hwrt INTERFACE)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../
)


target_sources(hwrt INTERFACE
    hwrt.c
)

target_link_libraries(hwrt INTERFACE
    pico_multicore
    pico_stdlib
)
