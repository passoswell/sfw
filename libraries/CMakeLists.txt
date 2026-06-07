
add_subdirectory(hal_interface)
add_subdirectory(device)
add_subdirectory(dsp)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_subdirectory(hal_linux)
endif()

if(SFW_PLATFORM STREQUAL "STM32")
  add_subdirectory(hal_stm32)
endif()