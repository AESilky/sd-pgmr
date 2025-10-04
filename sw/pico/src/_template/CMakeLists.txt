# Library: xyz - TEMPLATE (Library/Interface)
add_library(xyz INTERFACE)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../
)


target_sources(xyz INTERFACE
    xyz.c
)

target_link_libraries(xyz INTERFACE
    pico_stdlib
)
