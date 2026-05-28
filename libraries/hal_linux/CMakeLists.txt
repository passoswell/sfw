find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(LIBGPIOD REQUIRED IMPORTED_TARGET "libgpiod>=1.0" "libgpiod<2.0")
find_library(LIBI2C_LIBRARY NAMES i2c REQUIRED)

add_library(hal_linux)

target_link_libraries(hal_linux PUBLIC
  hal_interface
  Threads::Threads
  PkgConfig::LIBGPIOD
  ${LIBI2C_LIBRARY})

target_include_directories(hal_linux PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(HAL_LINUX_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_linux/dio/digital_input.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_linux/dio/digital_output.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_linux/i2c/i2c_controller.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_linux/spi/spi_controller.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_linux/software_timer/chrono_software_timer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_linux/uart/uart_serial.hpp
)

set(HAL_LINUX_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/digital_input.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/digital_output.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/i2c_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/spi_controller.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/chrono_software_timer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/uart_serial.cpp
)

target_sources(hal_linux PRIVATE ${HAL_LINUX_HEADERS} ${HAL_LINUX_SOURCES})

add_lint_anchor(
  hal_linux_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${HAL_LINUX_HEADERS}
  ${HAL_LINUX_SOURCES})

target_link_libraries(hal_linux_lint_anchor PRIVATE hal_linux)
