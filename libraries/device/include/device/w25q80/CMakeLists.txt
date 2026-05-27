add_library(w25q80 STATIC
  dev_mem_W25Q80.cpp
)

target_include_directories(w25q80 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_link_libraries(w25q80 PUBLIC hal_interface)

set(W25Q80_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/dev_mem_W25Q80_regs.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dev_mem_W25Q80.hpp
)

set(W25Q80_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/dev_mem_W25Q80.cpp
)

add_lint_anchor(
  w25q80_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${W25Q80_HEADERS}
  ${W25Q80_SOURCES})

target_link_libraries(w25q80_lint_anchor PRIVATE w25q80)