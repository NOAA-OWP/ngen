#[==[
Find the NetCDF C library, and optionally, the CXX/Fortran libraries.

  Hints:
    NETCDF_ROOT - prefix path to NetCDF development files, i.e. `/usr/local`

  Components:
    CXX     - Require the C++ interface
    FORTRAN - Require the Fortran interface

  Outputs:
    NetCDF_FOUND               - TRUE if NetCDF C was found
    NetCDF_VERSION             - NetCDF C version
    NetCDF_LIBRARY             - Library path to NetCDF C interface
    NetCDF_INCLUDE_DIR         - Include directory of C library
    NetCDF_HAS_PARALLEL        - TRUE if NetCDF C was compiled with ParallelIO support
    NetCDF_${LANG}_FOUND       - TRUE if component was found or available
    NetCDF_${LANG}_LIBRARY     - Library path of component
    NetCDF_${LANG}_INCLUDE_DIR - Include directory of component

  Targets:
    NetCDF          - INTERFACE target linking to C and component interfaces
    NetCDF::C       - IMPORTED target for C interface
    NetCDF::CXX     - IMPORTED target for C++ interface
    NetCDF::FORTRAN - IMPORTED target for Fortran Interface
#]==]

# NetCDF C Library ============================================================
find_path(NetCDF_INCLUDE_DIR
    NAMES netcdf.h
    PATHS "${NETCDF_ROOT}/include")
mark_as_advanced(NetCDF_C_INCLUDE_DIR)

find_library(NetCDF_LIBRARY
    NAMES netcdf
    PATHS "${NETCDF_ROOT}/lib"
    HINTS "${NetCDF_INCLUDE_DIR}/../lib")
mark_as_advanced(NetCDF_LIBRARY)

if(NetCDF_INCLUDE_DIR)
    # Get NetCDF Version
    file(STRINGS "${NetCDF_INCLUDE_DIR}/netcdf_meta.h" _netcdf_ver
        REGEX "#define[ \t]+NC_VERSION_(MAJOR|MINOR|PATCH|NOTE)")
    string(REGEX REPLACE ".*NC_VERSION_MAJOR *\([0-9]*\).*" "\\1" NetCDF_VERSION_MAJOR "${_netcdf_ver}")
    string(REGEX REPLACE ".*NC_VERSION_MINOR *\([0-9]*\).*" "\\1" NetCDF_VERSION_MINOR "${_netcdf_ver}")
    string(REGEX REPLACE ".*NC_VERSION_PATCH *\([0-9]*\).*" "\\1" NetCDF_VERSION_PATCH "${_netcdf_ver}")
    string(REGEX REPLACE ".*NC_VERSION_NOTE *\"\([^\"]*\)\".*" "\\1" _netcdf_version_note "${_netcdf_ver}")
    set(NetCDF_VERSION "${NetCDF_VERSION_MAJOR}.${NetCDF_VERSION_MINOR}.${NetCDF_VERSION_PATCH}${_netcdf_version_note}")
    unset(_netcdf_version_note)

    # Check if NetCDF was built with ParallelIO support
    file(STRINGS "${NetCDF_INCLUDE_DIR}/netcdf_meta.h" _netcdf_lines
        REGEX "#define[ \t]+NC_HAS_PARALLEL[ \t]")
    string(REGEX REPLACE ".*NC_HAS_PARALLEL[ \t]*([0-1]+).*" "\\1" _netcdf_has_parallel "${_netcdf_lines}")
    if (_netcdf_has_parallel)
        set(NetCDF_HAS_PARALLEL TRUE)
    else()
        set(NetCDF_HAS_PARALLEL FALSE)
    endif()
    unset(_netcdf_version_lines)
endif()

# NetCDF CXX Library ==========================================================
if("CXX" IN_LIST NetCDF_FIND_COMPONENTS)
    find_path(NetCDF_CXX_INCLUDE_DIR
        NAMES netcdf
        PATHS "${NETCDF_ROOT}/include")
    mark_as_advanced(NetCDF_CXX_INCLUDE_DIR)

    find_library(NetCDF_CXX_LIBRARY
        NAMES netcdf-cxx4 netcdf_c++4
        PATHS "${NETCDF_ROOT}/lib"
        HINTS "${NetCDF_CXX_INCLUDE_DIR}/../lib")
    mark_as_advanced(NetCDF_CXX_LIBRARY)
endif()

if(NetCDF_CXX_INCLUDE_DIR AND NetCDF_CXX_LIBRARY)
    set(NetCDF_CXX_FOUND TRUE)
else()
    set(NetCDF_CXX_FOUND FALSE)
endif()

# NetCDF Fortran Library ======================================================
if("FORTRAN" IN_LIST NetCDF_FIND_COMPONENTS)
    find_path(NetCDF_FORTRAN_INCLUDE_DIR
        NAMES netcdf.inc netcdf.mod
        PATHS "${NETCDF_ROOT}/include")
    mark_as_advanced(NetCDF_FORTRAN_INCLUDE_DIR)

    find_library(NetCDF_FORTRAN_LIBRARY
        NAMES netcdff
        PATHS "${NETCDF_ROOT}/lib"
        HINTS "${NetCDF_FORTRAN_INCLUDE_DIR}/../lib")
    mark_as_advanced(NetCDF_FORTRAN_LIBRARY)
endif()

if(NetCDF_FORTRAN_INCLUDE_DIR AND NetCDF_FORTRAN_LIBRARY)
    set(NetCDF_FORTRAN_FOUND TRUE)
else()
    set(NetCDF_FORTRAN_FOUND FALSE)
endif()

# =============================================================================
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NetCDF
  REQUIRED_VARS
    NetCDF_INCLUDE_DIR
    NetCDF_LIBRARY
  VERSION_VAR
    NetCDF_VERSION
  HANDLE_COMPONENTS
)

# NetCDF Targets ==============================================================
if(NetCDF_FOUND)
    add_library(NetCDF INTERFACE)
    add_library(NetCDF::NetCDF ALIAS NetCDF)

    add_library(NetCDF_C UNKNOWN IMPORTED)
    add_library(NetCDF::C ALIAS NetCDF_C)
    set_target_properties(NetCDF_C
        PROPERTIES
            IMPORTED_LOCATION "${NetCDF_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${NetCDF_INCLUDE_DIR}")
    target_link_libraries(NetCDF INTERFACE NetCDF::C)

    if("CXX" IN_LIST NetCDF_FIND_COMPONENTS)
        add_library(NetCDF_CXX UNKNOWN IMPORTED)
        add_library(NetCDF::CXX ALIAS NetCDF_CXX)
        set_target_properties(NetCDF_CXX
            PROPERTIES
                IMPORTED_LOCATION "${NetCDF_CXX_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${NetCDF_CXX_INCLUDE_DIR}")
        target_link_libraries(NetCDF INTERFACE NetCDF::CXX)
    endif()

    if("FORTRAN" IN_LIST NetCDF_FIND_COMPONENTS)
        add_library(NetCDF_FORTRAN UNKNOWN IMPORTED)
        add_library(NetCDF::FORTRAN ALIAS NetCDF_FORTRAN)
        set_target_properties(NetCDF_FORTRAN
            PROPERTIES
                IMPORTED_LOCATION "${NetCDF_FORTRAN_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${NetCDF_FORTRAN_INCLUDE_DIR}")
        target_link_libraries(NetCDF INTERFACE NetCDF::FORTRAN)
    endif()
endif()
