add_library(pcf8574 STATIC
  pcf8574.cpp
  pcf8574_digital_input.cpp
  pcf8574_digital_output.cpp
)

target_include_directories(pcf8574 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_link_libraries(pcf8574 PUBLIC hal_interface)

set(PCF8574_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/pcf8574.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pcf8574_digital_input.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pcf8574_digital_output.hpp
)

set(PCF8574_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/pcf8574.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pcf8574_digital_input.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pcf8574_digital_output.cpp
)

add_lint_anchor(
  pcf8574_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PCF8574_HEADERS}
  ${PCF8574_SOURCES})

target_link_libraries(pcf8574_lint_anchor PRIVATE pcf8574)