# Library: prog_device - TEMPLATE (Library/Interface)
add_library(prog_device INTERFACE)

target_include_directories(prog_device INTERFACE
	${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../
  ${PICO_SDK_PATH}/src/common/pico_base_headers/include
)


target_sources(prog_device INTERFACE
    prog_device.c
    pdops.c
)

target_link_libraries(prog_device INTERFACE
    pico_stdlib
)
