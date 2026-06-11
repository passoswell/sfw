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

add_lint_anchor(
  dsp_lint_anchor
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${DSP_HEADERS})