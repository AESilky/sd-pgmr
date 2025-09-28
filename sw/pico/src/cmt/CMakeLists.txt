# Library: cmt (obj only)
add_library(cmt INTERFACE)

# Add the required include file paths to the build
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${PICO_SDK_PATH}/src/rp2_common/pico_multicore/include
)

target_sources(cmt INTERFACE
  cmt.c
  cmt_heap.c
)

target_link_libraries(cmt INTERFACE
  pico_float
  pico_stdlib
)
