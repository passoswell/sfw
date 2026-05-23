function(enable_target_cppcheck TARGET_NAME)
    # Add cppcheck as a post-build step for an existing target.
    find_program(CPPCHECK_EXE NAMES cppcheck)
    if(CPPCHECK_EXE)
        set(ABS_SOURCES)
        foreach(SRC ${ARGN})
            if(NOT IS_ABSOLUTE ${SRC})
                set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
            endif()
            list(APPEND ABS_SOURCES ${SRC})
        endforeach()

        set(CPPCHECK_ARGS
            --enable=all
            --error-exitcode=1
            --inline-suppr
            --suppress=missingIncludeSystem
            --suppress=unusedFunction
            --std=c++${CMAKE_CXX_STANDARD}
            --project=${CMAKE_BINARY_DIR}/compile_commands.json
            ${ABS_SOURCES})

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CPPCHECK_EXE} ${CPPCHECK_ARGS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running cppcheck on ${TARGET_NAME}..."
            VERBATIM
            COMMAND_EXPAND_LISTS)
    else()
        message(FATAL_ERROR "cppcheck was not found in PATH. It is required for target '${TARGET_NAME}'.")
    endif()
endfunction()
