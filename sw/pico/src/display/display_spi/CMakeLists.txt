add_library(display_spi INTERFACE)

target_sources(display_spi INTERFACE
    display_spi_ops.c
)

target_link_libraries(display_spi INTERFACE
    pico_stdlib
    hardware_spi
)

