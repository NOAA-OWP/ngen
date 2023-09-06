macro(deprecated_option)
    set(options INCLUDE_ENV)
    set(oneValueArgs NEW OLD)
    cmake_parse_arguments(DEPRECATED_OPTION "${options}" "${oneValueArgs}" "" ${ARGN})

    if(DEFINED ${DEPRECATED_OPTION_OLD})
        message(DEPRECATION "Deprecated option ${DEPRECATED_OPTION_OLD} defined; setting ${DEPRECATED_OPTION_NEW} to ${DEPRECATED_OPTION_OLD} (aka ${${DEPRECATED_OPTION_OLD}})")
        set(${DEPRECATED_OPTION_NEW} ${${DEPRECATED_OPTION_OLD}})
    endif()

    if(DEPRECATED_OPTION_INCLUDE_ENV)
        if(DEFINED ENV{${DEPRECATED_OPTION_OLD}})
            message(DEPRECATION "Deprecated environment variable ${DEPRECATED_OPTION_OLD} defined; setting ${DEPRECATED_OPTION_NEW} to ENV{${DEPRECATED_OPTION_OLD}} (aka $ENV{${DEPRECATED_OPTION_OLD}})")
            set(${DEPRECATED_OPTION_NEW} $ENV{${DEPRECATED_OPTION_OLD}})
        endif()
    endif()
endmacro()

deprecated_option(OLD MPI_ACTIVE NEW NGEN_WITH_MPI INCLUDE_ENV)
