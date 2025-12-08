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
    # Modules that provide commands
    dbus_cmd
    debug_cmd
    disk_cmd
    pico_cmd
    prog_device_cmd
    # Pico Modules/Libraries used
    pico_stdlib
)

# Library: Shell STUB (do nothing shell_minit only)
add_library(shell_stub INTERFACE)

target_sources(shell_stub INTERFACE
    shell_stub.c
)
