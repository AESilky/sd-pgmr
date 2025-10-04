# Library: Disk Operations (higher level functionality for the SD and FATFS libraries)
add_library(dskops INTERFACE)

target_sources(dskops INTERFACE
    dskops.c
)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/../
  lib/sd_card/ff15/source/
)

target_link_libraries(dskops INTERFACE
    SD_FatFs
    pico_stdlib
)
