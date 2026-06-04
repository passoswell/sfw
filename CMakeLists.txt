cmake_minimum_required(VERSION 3.23)

include(cmake/preproject_configuration.cmake)
include(cmake/postproject_configuration.cmake)

project(sfw LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SFW_PLATFORM "Linux" CACHE STRING "Target platform for the build")
set_property(CACHE SFW_PLATFORM PROPERTY STRINGS Linux STM32)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_SOURCE_DIR}/cmake/AddLintedExecutable.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/AddClangFormat.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/AddCppcheck.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/AddLintAnchor.cmake)

sfw_apply_platform_preproject_configuration()

add_subdirectory(apps)
add_subdirectory(libraries)