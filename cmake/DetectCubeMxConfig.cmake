function(sfw_detect_cubemx_toolchain cubemx_dir out_defines out_cpu out_fpu out_float_abi)
  if(cubemx_dir STREQUAL "")
    message(FATAL_ERROR
      "SFW_STM32_CUBEMX_DIR must point to a CubeMX generated project")
  endif()

  set(CubeMxCmakeFile "${cubemx_dir}/cmake/stm32cubemx/CMakeLists.txt")
  set(CubeMxToolchainFile "${cubemx_dir}/cmake/gcc-arm-none-eabi.cmake")

  if(NOT EXISTS "${CubeMxCmakeFile}")
    message(FATAL_ERROR
      "CubeMX CMake file not found: ${CubeMxCmakeFile}")
  endif()

  if(NOT EXISTS "${CubeMxToolchainFile}")
    message(FATAL_ERROR
      "CubeMX toolchain file not found: ${CubeMxToolchainFile}")
  endif()

  file(READ "${CubeMxCmakeFile}" CubeMxCmakeContent)
  string(REGEX MATCH "set\\(MX_Defines_Syms[ \t\r\n]+([^\\)]*)\\)"
         DefinesMatch "${CubeMxCmakeContent}")
  set(DefinesBlock "${CMAKE_MATCH_1}")

  if(DefinesBlock STREQUAL "")
    message(FATAL_ERROR
      "Unable to parse MX_Defines_Syms from ${CubeMxCmakeFile}")
  endif()

  string(REPLACE "\r" "" DefinesBlock "${DefinesBlock}")
  string(REPLACE "\n" ";" DefineLines "${DefinesBlock}")

  set(DetectedDefines)
  foreach(DefineLine ${DefineLines})
    string(STRIP "${DefineLine}" DefineLine)
    if(DefineLine STREQUAL "")
      continue()
    endif()
    if(DefineLine MATCHES "^#")
      continue()
    endif()
    list(APPEND DetectedDefines "${DefineLine}")
  endforeach()

  if(DetectedDefines STREQUAL "")
    message(FATAL_ERROR
      "No compile definitions were extracted from ${CubeMxCmakeFile}")
  endif()

  file(READ "${CubeMxToolchainFile}" CubeMxToolchainContent)
  string(REGEX MATCH "set\\(TARGET_FLAGS \"([^\"]*)\"\\)"
         TargetFlagsMatch "${CubeMxToolchainContent}")
  set(TargetFlags "${CMAKE_MATCH_1}")

  if(TargetFlags STREQUAL "")
    message(FATAL_ERROR
      "Unable to parse TARGET_FLAGS from ${CubeMxToolchainFile}")
  endif()

  string(REGEX MATCH "-mcpu=([^ ]+)" CpuMatch "${TargetFlags}")
  set(DetectedCpu "${CMAKE_MATCH_1}")

  string(REGEX MATCH "-mfpu=([^ ]+)" FpuMatch "${TargetFlags}")
  set(DetectedFpu "${CMAKE_MATCH_1}")

  string(REGEX MATCH "-mfloat-abi=([^ ]+)" FloatAbiMatch "${TargetFlags}")
  set(DetectedFloatAbi "${CMAKE_MATCH_1}")

  if(DetectedCpu STREQUAL "")
    message(FATAL_ERROR
      "Unable to extract -mcpu from TARGET_FLAGS in ${CubeMxToolchainFile}")
  endif()

  if(DetectedFpu STREQUAL "")
    message(FATAL_ERROR
      "Unable to extract -mfpu from TARGET_FLAGS in ${CubeMxToolchainFile}")
  endif()

  if(DetectedFloatAbi STREQUAL "")
    message(FATAL_ERROR
      "Unable to extract -mfloat-abi from TARGET_FLAGS in ${CubeMxToolchainFile}")
  endif()

  set(${out_defines} "${DetectedDefines}" PARENT_SCOPE)
  set(${out_cpu} "${DetectedCpu}" PARENT_SCOPE)
  set(${out_fpu} "${DetectedFpu}" PARENT_SCOPE)
  set(${out_float_abi} "${DetectedFloatAbi}" PARENT_SCOPE)
endfunction()
