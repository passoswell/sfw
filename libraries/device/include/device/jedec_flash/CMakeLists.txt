add_library(jedec_spi_flash STATIC
  jedec_spi_flash.cpp
  jedec_spi_flash_memory.cpp
)

target_include_directories(jedec_spi_flash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_link_libraries(jedec_spi_flash PUBLIC hal_interface)

set(JEDEC_SPI_FLASH_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/jedec_flash_types.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/jedec_spi_flash.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/jedec_spi_flash_memory.hpp
)

set(JEDEC_SPI_FLASH_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/jedec_spi_flash.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/jedec_spi_flash_memory.cpp
)

add_lint_anchor(
  jedec_spi_flash_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${JEDEC_SPI_FLASH_HEADERS}
  ${JEDEC_SPI_FLASH_SOURCES})

target_link_libraries(jedec_spi_flash_lint_anchor PRIVATE jedec_spi_flash)
