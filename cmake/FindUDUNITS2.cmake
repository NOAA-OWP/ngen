if (UDUNITS2_INCLUDE_DIR AND UDUNITS2_LIBRARY)
    set(UDUNITS2_FIND_QUIETLY TRUE)
endif()

find_library(UDUNITS2_LIBRARY udunits2
    PATHS "${UDUNITS2_ROOT}/lib")
mark_as_advanced(UDUNITS2_LIBRARY)

get_filename_component(UDUNITS2_LIBRARY_DIR "${UDUNITS2_LIBRARY}" PATH)

find_path(UDUNITS2_INCLUDE_DIR udunits2.h
    PATHS "${UDUNITS2_ROOT}/include"
    HINTS "${UDUNITS2_LIBRARY_DIR}/../include"
    PATH_SUFFIXES "udunits2")
mark_as_advanced(UDUNITS2_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDUNITS2
    DEFAULT_MSG UDUNITS2_LIBRARY UDUNITS2_INCLUDE_DIR)

if(UDUNITS2_FOUND)
    # Create UDUNITS2 target
    # Note: GLOBAL is required here in order to extend the scope of the target.
    #       see: https://stackoverflow.com/a/46491758/6891484
    add_library(libudunits2 SHARED IMPORTED GLOBAL)
    target_link_libraries(libudunits2 INTERFACE "${UDUNITS2_LIBRARY}")
    target_include_directories(libudunits2 INTERFACE "${UDUNITS2_INCLUDE_DIR}")
    set_target_properties(libudunits2
        PROPERTIES
            IMPORTED_LOCATION "${UDUNITS2_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${UDUNITS2_INCLUDE_DIR}")
endif()
