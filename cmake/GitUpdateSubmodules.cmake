find_package(Git QUIET)

if(EXISTS "${NGEN_ROOT_DIR}/.git")
    set(NGEN_HAS_GIT_DIR ON)
else()
    set(NGEN_HAS_GIT_DIR OFF)
endif()

function(git_update_submodule PATH)
    if(NGEN_UPDATE_GIT_SUBMODULES)
        message(STATUS "Updating submodule ${PATH}")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init -- ${PATH}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT
            )
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init for \"${PATH}\" failed with ${GIT_SUBMOD_RESULT}")
        endif()
    else()
        message(FATAL_ERROR "Git not detected: cannot obtain submodule at \"${PATH}\" (but how is this even working?)")
    endif()
endfunction()
