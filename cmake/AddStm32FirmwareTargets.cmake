function(sfw_add_stm32_firmware_targets target_name)
  if(NOT TARGET ${target_name})
    message(FATAL_ERROR "Target ${target_name} does not exist")
  endif()

  if(NOT CMAKE_OBJCOPY)
    message(FATAL_ERROR "CMAKE_OBJCOPY is required for STM32 firmware builds")
  endif()

  if(NOT CMAKE_SIZE)
    message(FATAL_ERROR "CMAKE_SIZE is required for STM32 firmware builds")
  endif()

    set(FIRMWARE_ELF_PATH "${CMAKE_BINARY_DIR}/${target_name}.firmware.elf")

    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target_name}>
        ${FIRMWARE_ELF_PATH}
      COMMAND ${CMAKE_OBJCOPY} -O ihex ${FIRMWARE_ELF_PATH}
        $<TARGET_FILE_DIR:${target_name}>/${target_name}.hex
      COMMAND ${CMAKE_OBJCOPY} -O binary ${FIRMWARE_ELF_PATH}
        $<TARGET_FILE_DIR:${target_name}>/${target_name}.bin
      COMMAND ${CMAKE_SIZE} ${FIRMWARE_ELF_PATH}
      COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/tools/cmake-launch-target.sh
        $<TARGET_FILE:${target_name}>
      COMMAND chmod +x $<TARGET_FILE:${target_name}>
      VERBATIM)

  find_program(OPENOCD_EXECUTABLE openocd)
  set(OPENOCD_TARGET_SCRIPT "${CMAKE_BINARY_DIR}/openocd-target.auto.cfg")
  if(OPENOCD_EXECUTABLE)
    add_custom_target(${target_name}_flash_openocd
      COMMAND ${OPENOCD_EXECUTABLE}
              -f interface/stlink.cfg
              -f ${OPENOCD_TARGET_SCRIPT}
              -c "program ${FIRMWARE_ELF_PATH} verify reset exit"
      DEPENDS ${target_name}
      VERBATIM)
  endif()

  find_program(STM32_PROGRAMMER_CLI STM32_Programmer_CLI)
  if(STM32_PROGRAMMER_CLI)
    add_custom_target(${target_name}_flash_cubeprogrammer
      COMMAND ${STM32_PROGRAMMER_CLI} -c port=SWD
              -w ${FIRMWARE_ELF_PATH} -v -rst
      DEPENDS ${target_name}
      VERBATIM)
  endif()

  if(TARGET ${target_name}_flash_openocd)
    add_custom_target(${target_name}_flash
      DEPENDS ${target_name}_flash_openocd)
  elseif(TARGET ${target_name}_flash_cubeprogrammer)
    add_custom_target(${target_name}_flash
      DEPENDS ${target_name}_flash_cubeprogrammer)
  endif()
endfunction()