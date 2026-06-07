function(sfw_configure_platform_executable target_name)
	if(NOT TARGET ${target_name})
		message(FATAL_ERROR "Target ${target_name} does not exist")
	endif()

	if(SFW_PLATFORM STREQUAL "Linux")
		target_link_libraries(${target_name} PUBLIC hal_interface hal_linux device dsp)
		add_custom_command(TARGET ${target_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target_name}>
				${CMAKE_BINARY_DIR}/${target_name}.firmware.elf
			VERBATIM)
		return()
	endif()

	if(SFW_PLATFORM STREQUAL "STM32")
		set_target_properties(${target_name} PROPERTIES SUFFIX ".elf")
		target_link_libraries(${target_name} PUBLIC hal_interface hal_stm32 device dsp)
		target_compile_definitions(${target_name} PRIVATE SFW_STM32_BUILD=1)
		target_link_options(${target_name} PRIVATE
			-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target_name}.map)

		sfw_add_stm32_firmware_targets(${target_name})

		configure_file(
			${CMAKE_SOURCE_DIR}/tools/cmake-launch-target.sh
			${CMAKE_BINARY_DIR}/${target_name}
			COPYONLY)
		file(CHMOD ${CMAKE_BINARY_DIR}/${target_name}
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
									GROUP_READ GROUP_EXECUTE
									WORLD_READ WORLD_EXECUTE)
		return()
	endif()

	message(FATAL_ERROR "Unsupported SFW_PLATFORM: ${SFW_PLATFORM}")
endfunction()

function(sfw_enable_platform_checks target_name)
	if(NOT TARGET ${target_name})
		message(FATAL_ERROR "Target ${target_name} does not exist")
	endif()

	if(SFW_PLATFORM STREQUAL "Linux")
		enable_target_clang_format(${target_name} ${ARGN})
		enable_target_clang_tidy(${target_name} ${ARGN})
		enable_target_cppcheck(${target_name} ${ARGN})
		return()
	endif()

	if(SFW_PLATFORM STREQUAL "STM32")
		enable_target_clang_format(${target_name} ${ARGN})
		enable_target_clang_tidy(${target_name} ${ARGN})
		enable_target_cppcheck(${target_name} ${ARGN})
		return()
	endif()

	message(FATAL_ERROR "Unsupported SFW_PLATFORM: ${SFW_PLATFORM}")
endfunction()
