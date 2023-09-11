find_path(UDUNITS2_INCLUDE 
    NAMES udunits2.h
    PATH_SUFFIXES udunits2
    DOC "UDUNITS-2 include directories"
)
mark_as_advanced(UDUNITS2_INCLUDE)

find_library(UDUNITS2_LIBRARY
    NAMES
        libudunits2.so
        libudunits2.dylib
)
mark_as_advanced(UDUNITS2_LIBRARY)

if(DEFINED UDUNITS2_INCLUDE AND DEFINED UDUNITS2_LIBRARY)
    set(UDUNITS2_FOUND ON)
else()
    set(UDUNITS2_FOUND OFF)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDUNITS2 DEFAULT_MSG UDUNITS2_LIBRARY UDUNITS2_INCLUDE)

if(UDUNITS2_FOUND)
    # Create UDUNITS2 target
    # Note: GLOBAL is required here in order to extend the scope of the target.
    #       see: https://stackoverflow.com/a/46491758/6891484
    add_library(libudunits2 SHARED IMPORTED GLOBAL)
    set_target_properties(libudunits2 PROPERTIES IMPORTED_LOCATION "${UDUNITS2_LIBRARY}")
    target_include_directories(libudunits2 INTERFACE "${UDUNITS2_INCLUDE}")

    # Based on: https://stackoverflow.com/questions/26834553/osx-10-10-cmake-3-0-2-and-clang-wont-find-local-headers
    include_directories(${UDUNITS2_INCLUDE})
endif()
