# Library: cmd (command shell) (obj only)
add_library(cmds INTERFACE)

target_sources(cmds INTERFACE
    debug.c
    devrdwr.c
)

target_include_directories(cmds INTERFACE
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/../cmd
	${CMAKE_CURRENT_LIST_DIR}/../term
)

target_link_libraries(cmds INTERFACE
    cmd
    pico_stdlib
)
