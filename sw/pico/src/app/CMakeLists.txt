# Library: Application (APP) functionality (obj only)
add_library(app INTERFACE)

target_sources(app INTERFACE
  app.c
)

add_subdirectory(deviceops)
add_subdirectory(menumgr)
add_subdirectory(shell)


target_include_directories(app INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/deviceops/include
  ${CMAKE_CURRENT_LIST_DIR}/menumgr/include
  ${CMAKE_CURRENT_LIST_DIR}/shell/include
)

target_link_libraries(app INTERFACE
  dskops
  menumgr
  shell
  #shell_stub
  prog_device
  pico_stdlib
)

