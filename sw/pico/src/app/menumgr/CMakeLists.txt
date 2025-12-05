# Library: menumgr - Menu Manager for UI functionality (Library/Interface)
add_library(menumgr INTERFACE)

target_include_directories(menumgr INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(menumgr INTERFACE
  menumgr.c
)

target_link_libraries(menumgr INTERFACE
  cmt
  pico_stdlib
)
