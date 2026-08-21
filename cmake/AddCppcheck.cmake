function(enable_target_cppcheck TARGET_NAME)
    # Add cppcheck as a post-build step for an existing target.
    find_program(CPPCHECK_EXE NAMES cppcheck)
    if(CPPCHECK_EXE)
        set(ABS_SOURCES)
        foreach(SRC ${ARGN})
            if(NOT IS_ABSOLUTE ${SRC})
                set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
            endif()
            if(NOT SRC MATCHES "^${CMAKE_SOURCE_DIR}/external/.*")
                list(APPEND ABS_SOURCES ${SRC})
            endif()
        endforeach()

        if(NOT ABS_SOURCES)
            message(STATUS "Skipping cppcheck for ${TARGET_NAME}: no non-external sources.")
            return()
        endif()

        set(SFW_CPPCHECK_EXCLUDES -i${CMAKE_SOURCE_DIR}/external)
        if(SFW_PLATFORM STREQUAL "STM32" AND NOT SFW_STM32_CUBEMX_DIR STREQUAL "")
            list(APPEND SFW_CPPCHECK_EXCLUDES -i${SFW_STM32_CUBEMX_DIR})
        endif()

        set(SFW_CPPCHECK_COMPILER_DEFINES)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
            list(APPEND SFW_CPPCHECK_COMPILER_DEFINES
                -D__GNUC__=1
                -D__GNUC_MINOR__=0
                -D__GNUC_PATCHLEVEL__=0)
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
            list(APPEND SFW_CPPCHECK_COMPILER_DEFINES -D__clang__=1)
        endif()

        set(CPPCHECK_ARGS
            --enable=all
            --error-exitcode=1
            --inline-suppr
            --suppress=*:${CMAKE_SOURCE_DIR}/external/*
            --suppress=missingIncludeSystem
            --suppress=unusedFunction
            --suppress=unmatchedSuppression
            --std=c++${CMAKE_CXX_STANDARD}
            --project=${CMAKE_BINARY_DIR}/compile_commands.json
            ${SFW_CPPCHECK_COMPILER_DEFINES}
            ${SFW_CPPCHECK_EXCLUDES}
            ${ABS_SOURCES})

        # Restrict project-mode analysis to this target's translation units.
        # Without file filters, cppcheck scans the entire compile database,
        # including third-party code not owned by this target.
        set(CPPCHECK_FILE_FILTERS)
        foreach(SRC ${ABS_SOURCES})
            list(APPEND CPPCHECK_FILE_FILTERS --file-filter=${SRC})
        endforeach()

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CPPCHECK_EXE} ${CPPCHECK_ARGS} ${CPPCHECK_FILE_FILTERS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running cppcheck on ${TARGET_NAME}..."
            VERBATIM
            COMMAND_EXPAND_LISTS)
    else()
        message(FATAL_ERROR "cppcheck was not found in PATH. It is required for target '${TARGET_NAME}'.")
    endif()
endfunction()
