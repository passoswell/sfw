function(enable_target_clang_tidy TARGET_NAME)
    # Add clang-tidy as a post-build step for an existing target.
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
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
            message(STATUS "Skipping clang-tidy for ${TARGET_NAME}: no non-external sources.")
            return()
        endif()

        set(CLANG_TIDY_EXTRA_ARGS "")
        foreach(INC_DIR ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
            list(APPEND CLANG_TIDY_EXTRA_ARGS "--extra-arg=-isystem${INC_DIR}")
        endforeach()
        list(APPEND CLANG_TIDY_EXTRA_ARGS "--extra-arg=-std=c++${CMAKE_CXX_STANDARD}")

        set(SFW_CLANG_TIDY_HEADER_FILTER
            "^${CMAKE_SOURCE_DIR}/(apps|libraries)/.*")
        if(SFW_PLATFORM STREQUAL "STM32")
            set(SFW_CLANG_TIDY_HEADER_FILTER
                "^${CMAKE_SOURCE_DIR}/(apps|libraries)/.*")
        endif()

        set(CLANG_TIDY_ARGS
            --fix
            --fix-errors
            --quiet
            --warnings-as-errors=*
            --header-filter=${SFW_CLANG_TIDY_HEADER_FILTER}
            -p=${CMAKE_BINARY_DIR}
            ${ABS_SOURCES}
            ${CLANG_TIDY_EXTRA_ARGS})

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CLANG_TIDY_EXE} ${CLANG_TIDY_ARGS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running clang-tidy checks on ${TARGET_NAME}..."
            VERBATIM
            COMMAND_EXPAND_LISTS)
    else()
        message(FATAL_ERROR "clang-tidy was not found in PATH. It is required for target '${TARGET_NAME}'.")
    endif()
endfunction()