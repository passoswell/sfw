# Use FetchContent to automate downloading the CMSIS-DSP source code
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/external")
include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
  cmsis_dsp
  GIT_REPOSITORY https://github.com/ARM-software/CMSIS-DSP.git
  GIT_TAG        main # Or use a specific version tag like v1.15.0
  GIT_PROGRESS TRUE
)

# Force CMSIS-DSP to build natively on your host Linux environment
set(HOST ON CACHE INTERNAL "")
set(DISABLE_TESTING ON CACHE INTERNAL "")

FetchContent_MakeAvailable(cmsis_dsp)

add_library(dsp INTERFACE)

target_include_directories(dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Hook targets require a non-INTERFACE target for POST_BUILD commands.
set(DSP_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dsp/interface/filter.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dsp/fir/fir_direct_form.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dsp/fir/fir_transposed_form.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/dsp/iir/iir_biquad.hpp
)

target_sources(dsp INTERFACE ${DSP_HEADERS})


target_link_libraries(dsp INTERFACE CMSISDSP::CMSISDSP)

add_lint_anchor(
  dsp_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${DSP_HEADERS})