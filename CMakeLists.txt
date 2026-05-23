cmake_minimum_required(VERSION 3.23)

project(sfw LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Werror -Wall -Wextra)

include(${CMAKE_SOURCE_DIR}/cmake/AddLintedExecutable.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/AddClangFormat.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/AddCppcheck.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/AddLintAnchor.cmake)

add_subdirectory(libraries)

add_executable(sfw sfw.cpp)
target_link_libraries(sfw PUBLIC hal_interface hal_linux device)
enable_target_clang_format(sfw sfw.cpp)
enable_target_clang_tidy(sfw sfw.cpp)
enable_target_cppcheck(sfw sfw.cpp)