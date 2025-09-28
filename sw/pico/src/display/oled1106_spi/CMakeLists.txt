add_library(display INTERFACE)

target_sources(display INTERFACE
  display_oled1106.c
  oled1106_spi.c
)

include_directories(
    ../display_spi
    ../fonts
)

target_link_libraries(display INTERFACE
  display_spi
  fonts
  pico_stdlib
  hardware_spi
)
