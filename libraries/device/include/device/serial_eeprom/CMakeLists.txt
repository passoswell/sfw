add_library(serial_eeprom STATIC
  serial_eeprom.cpp
  serial_eeprom_i2c_transport.cpp
)

target_include_directories(serial_eeprom
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(serial_eeprom PUBLIC hal_interface)

set(SERIAL_EEPROM_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/interface/serial_eeprom_transport_interface.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/serial_eeprom.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/serial_eeprom_i2c_transport.hpp
)

set(SERIAL_EEPROM_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/serial_eeprom.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/serial_eeprom_i2c_transport.cpp
)

add_lint_anchor(
  serial_eeprom_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${SERIAL_EEPROM_HEADERS}
  ${SERIAL_EEPROM_SOURCES})

target_link_libraries(serial_eeprom_lint_anchor PRIVATE serial_eeprom)
