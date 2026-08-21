function(enable_target_clang_format TARGET_NAME)
    find_program(CLANG_FORMAT_EXE NAMES clang-format)
    if(CLANG_FORMAT_EXE)
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
            message(STATUS "Skipping clang-format for ${TARGET_NAME}: no non-external sources.")
            return()
        endif()

        add_custom_target(format_${TARGET_NAME}
            COMMAND ${CLANG_FORMAT_EXE} -i --style=file ${ABS_SOURCES}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running clang-format on ${TARGET_NAME} sources..."
            VERBATIM)

        add_dependencies(${TARGET_NAME} format_${TARGET_NAME})
    else()
        message(FATAL_ERROR "clang-format was not found in PATH. It is required for target '${TARGET_NAME}'.")
    endif()
endfunction()
