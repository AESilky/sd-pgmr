#
# Supplies the implementation of the 'display' interface/library/module
#
add_library(display INTERFACE)

target_sources(display INTERFACE
  oled1106.c
  oled1106_spi.c
)

include_directories(
    ../dispops
    ../fonts
)

target_link_libraries(display INTERFACE
  dispops
  fonts
  pico_stdlib
  hardware_spi
)
