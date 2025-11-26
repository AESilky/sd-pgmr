# Library: Interactive Shell (obj only)
add_library(shell INTERFACE)

target_sources(shell INTERFACE
    shell.c
)

add_subdirectory(cmd)
add_subdirectory(term)

target_include_directories(shell INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
	${CMAKE_CURRENT_LIST_DIR}/cmd
	${CMAKE_CURRENT_LIST_DIR}/term
)

target_link_libraries(shell INTERFACE
    cmd
    term
    pico_stdlib
)
