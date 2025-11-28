#
# Supplies the implementation of the 'display' interface/library/module
#
add_library(disp_oled1106_spi INTERFACE)

target_sources(disp_oled1106_spi INTERFACE
  oled1106.c
  oled1106_spi.c
)

target_include_directories(disp_oled1106_spi INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${CMAKE_CURRENT_LIST_DIR}/../dispops
    ${CMAKE_CURRENT_LIST_DIR}/../fonts
)

target_link_libraries(disp_oled1106_spi INTERFACE
  dispops
  fonts
  pico_stdlib
  hardware_spi
)
