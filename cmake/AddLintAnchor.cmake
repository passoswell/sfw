# add_lint_anchor(TARGET_NAME INCLUDE_DIR [HEADERS...])
#
# Creates a static library named TARGET_NAME from a generated .cpp file that
# includes all provided HEADERS. Lint tools (clang-format, clang-tidy, cppcheck)
# are then attached to this target so INTERFACE libraries can be linted via a
# single translation unit rather than one per header.
#
# Parameters:
#   TARGET_NAME  - Name of the static library target to create.
#   INCLUDE_DIR  - Directory added as a PRIVATE include path to the target.
#                  Header #include directives are computed relative to this dir.
#   HEADERS      - Absolute paths to the headers to lint (variadic).
function(add_lint_anchor TARGET_NAME INCLUDE_DIR)
  set(HEADERS ${ARGN})

  set(ANCHOR_CPP ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.cpp)

  set(ANCHOR_CONTENT "// Copyright (c) 2026 sfw contributors. All rights reserved.\n")
  string(APPEND ANCHOR_CONTENT "// Generated lint anchor — do not edit.\n")
  foreach(HDR ${HEADERS})
    file(RELATIVE_PATH HDR_REL "${INCLUDE_DIR}" "${HDR}")
    string(APPEND ANCHOR_CONTENT "#include \"${HDR_REL}\"\n")
  endforeach()
  file(WRITE "${ANCHOR_CPP}" "${ANCHOR_CONTENT}")

  add_library(${TARGET_NAME} STATIC "${ANCHOR_CPP}")
  target_include_directories(${TARGET_NAME} PRIVATE "${INCLUDE_DIR}")

  # clang-format operates directly on the header sources.
  enable_target_clang_format(${TARGET_NAME} ${HEADERS})
  # clang-tidy and cppcheck run on the single anchor TU; --header-filter
  # surfaces diagnostics in the included headers without extra TUs.
  enable_target_clang_tidy(${TARGET_NAME} "${ANCHOR_CPP}")
  enable_target_cppcheck(${TARGET_NAME} "${ANCHOR_CPP}")
endfunction()
