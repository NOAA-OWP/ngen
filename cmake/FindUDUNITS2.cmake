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

if(DEFINED UDUNITS2_INCLUDE)
    set(UDUNITS2_FOUND ON)
else()
    set(UDUNITS2_FOUND OFF)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDUNITS2 DEFAULT_MSG UDUNITS2_LIBRARY UDUNITS2_INCLUDE)
