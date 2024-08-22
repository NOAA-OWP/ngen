find_package(Git QUIET)

if(EXISTS "${NGEN_ROOT_DIR}/.git")
    set(NGEN_HAS_GIT_DIR ON)

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${NGEN_ROOT_DIR}
        OUTPUT_VARIABLE NGEN_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(NGEN_HAS_GIT_DIR OFF)
endif()

function(git_update_submodule PATH)
    if(NGEN_UPDATE_GIT_SUBMODULES)
        message(STATUS "Updating submodule ${PATH}")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${PATH}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT
            )
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init for \"${PATH}\" failed with ${GIT_SUBMOD_RESULT}")
        endif()
    endif()
endfunction()
