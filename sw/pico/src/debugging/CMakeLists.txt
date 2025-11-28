# Library: Debug Operations (obj only)
add_library(debugging INTERFACE)

target_sources(debugging INTERFACE
    debug_hw.c
	debug_support.c
)

target_include_directories(debugging INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

add_subdirectory(cmd)

target_link_libraries(debugging INTERFACE
    pico_stdlib
)
