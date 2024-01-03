include(GitUpdateSubmodules)

# Add an external subdirectory to build
function(add_external_subdirectory)
    set(options "")
    set(oneValueArgs SOURCE OUTPUT GIT_UPDATE IMPORTS)
    set(multiValueArgs "")
    cmake_parse_arguments(EXTERNAL_TARGET
        "${options}" "${oneValueArgs}"
        "${multiValueArgs}" ${ARGN})

    if(NGEN_UPDATE_GIT_SUBMODULES AND EXTERNAL_TARGET_GIT_UPDATE)
        git_update_submodule("${EXTERNAL_TARGET_GIT_UPDATE}")
    endif()

    if(EXTERNAL_TARGET_OUTPUT STREQUAL "")
        add_subdirectory("${EXTERNAL_TARGET_SOURCE}")
    else()
        add_subdirectory("${EXTERNAL_TARGET_SOURCE}" "${EXTERNAL_TARGET_OUTPUT}")
    endif()

    if(EXTERNAL_TARGET_IMPORTS STREQUAL "")
        if(TARGET ${EXTERNAL_TARGET_IMPORTS})
            set_target_properties(${EXTERNAL_TARGET_IMPORTS}
                PROPERTIES
                    CXX_VISIBILITY_PRESET default)
        else()
            message(FATAL_ERROR "Target `${EXTERNAL_TARGET_IMPORTS}` could not be found from subdirectory ${EXTERNAL_TARGET_SOURCE}")
        endif()
    endif()
endfunction()
