message("Looking for UDUNITS-2...")
find_path(UDUNITS2_INCLUDE 
    NAMES udunits2.h
    PATH_SUFFIXES udunits2
    DOC "UDUNITS-2 include directories"
)
mark_as_advanced(UDUNITS2_INCLUDE)
if(UDUNITS2_INCLUDE)
    include_directories(${UDUNITS2_INCLUDE})
endif()

find_library(UDUNITS2_LIBRARY
    NAMES libudunits2.so libudunits2.dylib
)
mark_as_advanced(UDUNITS2_LIBRARY)
if(UDUNITS2_LIBRARY)
    add_library(libudunits2 SHARED IMPORTED)
    set_property(TARGET libudunits2 PROPERTY IMPORTED_LOCATION "${UDUNITS2_LIBRARY}")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(UDUNITS2 DEFAULT_MSG UDUNITS2_LIBRARY UDUNITS2_INCLUDE)