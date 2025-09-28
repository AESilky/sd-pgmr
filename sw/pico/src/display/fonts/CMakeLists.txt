add_library(fonts INTERFACE)

target_sources(fonts INTERFACE
  font_9_10_h.c
)

target_link_libraries(fonts INTERFACE
  pico_stdlib
)
