# Add a fortran library to the project. Exhibits same behavior of `add_library()`,
# with the exception of setting the Fortran module directory, and including it
# on the target with an INTERFACE scope.
#
# add_fortran_library(<name> [STATIC | SHARED | MODULE]
#                     [EXCLUDE_FROM_ALL] [<source>...])
#
# A target ``<name>`` is created after calling this function, and
# normal target functions, i.e. `target_sources()`, can be used on it.
function(add_fortran_library libname)
    add_library(${libname} ${ARGN})
    get_target_property(_binary_dir ${libname} BINARY_DIR)
    set_target_properties(${libname}
        PROPERTIES
            Fortran_MODULE_DIRECTORY ${_binary_dir}/mod)
    target_include_directories(${libname}
        INTERFACE
            "${_binary_dir}/mod")
endfunction()
