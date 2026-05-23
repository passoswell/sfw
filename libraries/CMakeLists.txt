
add_subdirectory(hal_interface)
add_subdirectory(device)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_subdirectory(hal_linux)
endif()