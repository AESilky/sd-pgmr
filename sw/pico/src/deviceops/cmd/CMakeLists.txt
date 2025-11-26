# Library: prog_device - TEMPLATE (Library/Interface)
add_library(prog_device_cmd INTERFACE)

target_sources(prog_device_cmd INTERFACE
  cmds.c
)

target_include_directories(prog_device_cmd INTERFACE
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/shell/include
	${CMAKE_CURRENT_LIST_DIR}/../include
)

target_link_libraries(prog_device_cmd INTERFACE
    cmd
)
