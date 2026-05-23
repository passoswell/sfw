add_library(hal_interface INTERFACE)

target_include_directories(hal_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Hook targets require a non-INTERFACE target for POST_BUILD commands.
set(HAL_INTERFACE_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/analog_input.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/analog_output.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/digital_inout.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/digital_input.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/digital_output.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/error_code.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/i2c_controller.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/memory.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/pwm.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/serial.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/software_timer.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/spi.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_interface/timer.hpp
)

target_sources(hal_interface INTERFACE ${HAL_INTERFACE_HEADERS})

add_lint_anchor(
  hal_interface_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${HAL_INTERFACE_HEADERS})