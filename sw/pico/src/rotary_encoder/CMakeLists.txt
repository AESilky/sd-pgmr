# Library: rotary_encoder (Includes Programable I/O)
add_library(rotary_encoder INTERFACE)

pico_generate_pio_header(
    rotary_encoder ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated
)

target_sources(rotary_encoder INTERFACE
    rotary_encoder.c
    re_pbsw.c
)

target_include_directories(rotary_encoder INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${PICO_SDK_PATH}/common/pico_base_headers/include
)

target_link_libraries(rotary_encoder INTERFACE
    hardware_pio
    pico_stdlib
)
