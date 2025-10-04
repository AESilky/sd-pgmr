# Library: util (obj only)
add_library(util INTERFACE)

target_sources(util INTERFACE
  util.c
)

target_link_libraries(util INTERFACE
  pico_stdlib
)
