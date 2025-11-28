# Library: xyz - TEMPLATE (Library/Interface)
add_library(dbus INTERFACE)

target_include_directories(dbus INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(dbus INTERFACE
    dbus.c
)

target_link_libraries(dbus INTERFACE
    pico_stdlib
)

add_subdirectory(cmd)
