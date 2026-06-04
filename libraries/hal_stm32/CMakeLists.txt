include(${CMAKE_SOURCE_DIR}/cmake/LinkCubeMxProject.cmake)

add_library(sfw_stm32_platform INTERFACE)
target_compile_definitions(sfw_stm32_platform INTERFACE
  ${SFW_STM32_DEFINES})
target_compile_options(sfw_stm32_platform INTERFACE
  -mcpu=${SFW_STM32_CPU}
  -mthumb
  -mfpu=${SFW_STM32_FPU}
  -mfloat-abi=${SFW_STM32_FLOAT_ABI}
  $<$<COMPILE_LANGUAGE:C>:-Wno-error=volatile>  # Disabling volatile warnings for C files, as CubeMX generated code often has inconsistencies with the linting tools configured.
  $<$<COMPILE_LANGUAGE:CXX>:-Werror -Wall -Wextra>
  -ffunction-sections
  -fdata-sections)
target_link_options(sfw_stm32_platform INTERFACE
  -mcpu=${SFW_STM32_CPU}
  -mthumb
  -mfpu=${SFW_STM32_FPU}
  -mfloat-abi=${SFW_STM32_FLOAT_ABI}
  --specs=nano.specs
  --specs=nosys.specs
  -Wl,--gc-sections)
target_link_libraries(sfw_stm32_platform INTERFACE m)

sfw_add_stm32_cubemx_target(sfw_stm32_cubemx "${SFW_STM32_CUBEMX_DIR}"
                            "${SFW_STM32_DEFINES}")
target_link_libraries(sfw_stm32_cubemx PUBLIC sfw_stm32_platform)

add_library(hal_stm32)
target_link_libraries(hal_stm32 PUBLIC
  hal_interface
  -Wl,--whole-archive
  sfw_stm32_cubemx
  -Wl,--no-whole-archive
  sfw_stm32_platform)
target_include_directories(hal_stm32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(HAL_STM32_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/hal_stm32/dio/digital_output.hpp)

set(HAL_STM32_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/digital_output.cpp)

target_sources(hal_stm32 PRIVATE ${HAL_STM32_HEADERS} ${HAL_STM32_SOURCES})

add_lint_anchor(
  hal_stm32_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${HAL_STM32_HEADERS}
  ${HAL_STM32_SOURCES})

target_link_libraries(hal_stm32_lint_anchor PRIVATE hal_stm32)