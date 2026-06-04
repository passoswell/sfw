function(sfw_add_stm32_cubemx_target target_name cubemx_dir defines)
  if(cubemx_dir STREQUAL "")
    message(FATAL_ERROR
      "SFW_STM32_CUBEMX_DIR must point to a CubeMX generated project")
  endif()

  if(NOT EXISTS "${cubemx_dir}/Core/Inc")
    message(FATAL_ERROR
      "CubeMX project not found at ${cubemx_dir}: missing Core/Inc")
  endif()

  file(GLOB_RECURSE CubeMxSources CONFIGURE_DEPENDS
    "${cubemx_dir}/*.c"
    "${cubemx_dir}/*.s"
    "${cubemx_dir}/*.S")

  list(FILTER CubeMxSources EXCLUDE REGEX "/build/")
  list(FILTER CubeMxSources EXCLUDE REGEX "/cmake/")
  # list(FILTER CubeMxSources EXCLUDE REGEX "/main\\.c$")

  file(GLOB_RECURSE CubeMxHeaders CONFIGURE_DEPENDS
    "${cubemx_dir}/*.h")

  set(CubeMxIncludeDirs)
  foreach(HeaderFile ${CubeMxHeaders})
    if(HeaderFile MATCHES "/build/" OR HeaderFile MATCHES "/cmake/")
      continue()
    endif()
    get_filename_component(HeaderDir "${HeaderFile}" DIRECTORY)
    list(APPEND CubeMxIncludeDirs "${HeaderDir}")
  endforeach()
  list(REMOVE_DUPLICATES CubeMxIncludeDirs)

  file(GLOB CubeMxLinkerScripts "${cubemx_dir}/*_FLASH.ld")
  list(LENGTH CubeMxLinkerScripts CubeMxLinkerScriptCount)

  if(CubeMxLinkerScriptCount EQUAL 0)
    message(FATAL_ERROR
      "No CubeMX linker script matching *_FLASH.ld was found in ${cubemx_dir}")
  endif()

  list(GET CubeMxLinkerScripts 0 CubeMxLinkerScript)

  set(ProcessedLinkerScript
      "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_linker.ld")
  file(READ "${CubeMxLinkerScript}" LinkerScriptContent)
  string(REPLACE " (READONLY)" "" LinkerScriptContent
                 "${LinkerScriptContent}")
  file(WRITE "${ProcessedLinkerScript}" "${LinkerScriptContent}")

  add_library(${target_name} STATIC)
  target_sources(${target_name} PRIVATE ${CubeMxSources})
  target_include_directories(${target_name} SYSTEM PUBLIC ${CubeMxIncludeDirs})
  target_compile_definitions(${target_name} PUBLIC ${defines})
  target_link_options(${target_name} INTERFACE -T${ProcessedLinkerScript})
endfunction()