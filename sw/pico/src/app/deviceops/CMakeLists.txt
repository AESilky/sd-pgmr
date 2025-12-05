# Library: prog_device - TEMPLATE (Library/Interface)
add_library(prog_device INTERFACE)

target_sources(prog_device INTERFACE
    prog_device.c
    pdops.c
)

add_subdirectory(cmd)

target_include_directories(prog_device INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(prog_device INTERFACE
    pico_stdlib
)
