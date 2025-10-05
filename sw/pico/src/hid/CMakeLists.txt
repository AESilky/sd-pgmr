# Library: Human Interface Device (HID) functionality (obj only)
add_library(hid INTERFACE)

target_sources(hid INTERFACE
  hid.c
)

target_link_libraries(prog_device INTERFACE
    dskops
    prog_device
    pico_stdlib
)

