#[==[
deprecated_option(NEW <variable> OLD <variable> [INCLUDE_ENV])

Adds a CMake option that will output a deprecation message on use.

The `NEW` variable is the option that should be used when building.

The `OLD` variable is the deprecated option that corresponds to a new option.

The `INCLUDE_ENV` flag also checks the environment variables for a variable named `OLD`, i.e. ENV{<OLD variable>}.
#]==]
function(deprecated_option)
    set(options INCLUDE_ENV)
    set(oneValueArgs NEW OLD)
    cmake_parse_arguments(DEPRECATED_OPTION "${options}" "${oneValueArgs}" "" ${ARGN})

    if(DEFINED ${DEPRECATED_OPTION_OLD})
        message(DEPRECATION "Deprecated option ${DEPRECATED_OPTION_OLD} defined; setting ${DEPRECATED_OPTION_NEW} to ${DEPRECATED_OPTION_OLD} (aka ${${DEPRECATED_OPTION_OLD}})")
        set(${DEPRECATED_OPTION_NEW} ${${DEPRECATED_OPTION_OLD}} PARENT_SCOPE)
    endif()

    if(DEPRECATED_OPTION_INCLUDE_ENV)
        if(DEFINED ENV{${DEPRECATED_OPTION_OLD}})
            message(DEPRECATION "Deprecated environment variable ${DEPRECATED_OPTION_OLD} defined; setting ${DEPRECATED_OPTION_NEW} to ENV{${DEPRECATED_OPTION_OLD}} (aka $ENV{${DEPRECATED_OPTION_OLD}})")
            set(${DEPRECATED_OPTION_NEW} $ENV{${DEPRECATED_OPTION_OLD}} PARENT_SCOPE)
        endif()
    endif()
endfunction()
