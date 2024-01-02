include(GitUpdateSubmodules)

# Add an external subdirectory to build
function(add_external_subdirectory)
    set(options "")
    set(oneValueArgs SOURCE OUTPUT GIT_UPDATE)
    set(multiValueArgs "")
    cmake_parse_arguments(EXTERNAL_TARGET
        "${options}" "${oneValueArgs}"
        "${multiValueArgs}" ${ARGN})

    if(NGEN_UPDATE_GIT_SUBMODULES AND EXTERNAL_TARGET_GIT_UPDATE)
        git_update_submodule("${EXTERNAL_TARGET_GIT_UPDATE}")
    endif()

    if("EXTERNAL_TARGET_OUTPUT" IN_LIST EXTERNAL_TARGET_MISSING_VALUES)
        add_subdirectory("${EXTERNAL_TARGET_SOURCE}")
    else()
        add_subdirectory("${EXTERNAL_TARGET_SOURCE}" "${EXTERNAL_TARGET_OUTPUT}")
    endif()
endfunction()
