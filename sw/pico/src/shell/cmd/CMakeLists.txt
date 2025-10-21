# Library: cmd (command shell) (obj only)
add_library(cmd INTERFACE)

target_sources(cmd INTERFACE
    cmd.c
)

target_link_libraries(cmd INTERFACE
    pico_stdlib
)
