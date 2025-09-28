add_library(dispops INTERFACE)

target_sources(dispops INTERFACE
    dispspi_ops.c
)

target_link_libraries(dispops INTERFACE
    pico_stdlib
    hardware_spi
)

