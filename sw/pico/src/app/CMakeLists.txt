# Library: Application (APP) functionality (obj only)
add_library(app INTERFACE)

target_sources(app INTERFACE
  app.c
)

target_link_libraries(app INTERFACE
    dskops
    shell
    prog_device
    pico_stdlib
)

