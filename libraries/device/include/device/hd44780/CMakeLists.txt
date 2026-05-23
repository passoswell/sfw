add_library(hd44780 STATIC
  hd44780.cpp
  hd44780_bus_4_bits.cpp
  hd44780_bus_8_bits.cpp
)

target_include_directories(hd44780 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_link_libraries(hd44780 PUBLIC hal_interface)

set(HD44780_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/hd44780.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/interface/hd44780_bus_interface.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hd44780_bus_4_bits.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hd44780_bus_8_bits.hpp
)

set(HD44780_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/hd44780.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hd44780_bus_4_bits.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hd44780_bus_8_bits.cpp
)

add_lint_anchor(
  hd44780_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${HD44780_HEADERS}
  ${HD44780_SOURCES})

target_link_libraries(hd44780_lint_anchor PRIVATE hd44780)