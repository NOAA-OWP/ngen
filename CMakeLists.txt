# Ensure CMake policies have defaults depending on the CMake version used
# between the two versions specified. e.g. if 3.18 is used, then 3.18 defaults
# will be used instead of 3.17 defaults.
cmake_minimum_required(VERSION 3.17...3.26)

# Policies ====================================================================

# Use `LOCATION` for python lookup strategy
# https://cmake.org/cmake/help/latest/policy/CMP0094.html
cmake_policy(SET CMP0094 NEW)

# find_package() uses <PackageName>_ROOT variables.
# https://cmake.org/cmake/help/latest/policy/CMP0074.html
cmake_policy(SET CMP0074 NEW)

# find_package() uses upper-case <PACKAGENAME>_ROOT variables.
# https://cmake.org/cmake/help/latest/policy/CMP0144.html
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

# Project Variables ===========================================================
set(NGEN_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(NGEN_SRC_DIR  "${NGEN_ROOT_DIR}/src")
set(NGEN_INC_DIR  "${NGEN_ROOT_DIR}/include")
set(NGEN_EXT_DIR  "${NGEN_ROOT_DIR}/extern")
set(NGEN_MOD_DIR  "${NGEN_ROOT_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${NGEN_MOD_DIR}")

# Provides:
#   (1) finds Git package
#   (2) Sets ${NGEN_HAS_GIT_DIR} to ON/OFF if ${NGEN_ROOT_DIR}/.git exists
#   (3) Function `git_update_submodule` which updates a given submodule
include(GitUpdateSubmodules)

# Options =====================================================================
include(CMakeDependentOption)

option(NGEN_WITH_MPI         "Build with MPI support"             OFF)
option(NGEN_WITH_NETCDF      "Build with NetCDF support"          ON)
option(NGEN_WITH_SQLITE      "Build with SQLite3 support"         OFF)
option(NGEN_WITH_UDUNITS     "Build with UDUNITS2 support"        ON)
option(NGEN_WITH_BMI_FORTRAN "Build with Fortran BMI support"     OFF)
option(NGEN_WITH_BMI_C       "Build with C BMI support"           OFF)
option(NGEN_WITH_PYTHON      "Build with embedded Python support" OFF)
option(NGEN_WITH_TESTS       "Build with unit tests"              ON)
option(NGEN_QUIET            "Silence output"                     OFF)

# These options require dependency on some of the above options
# Syntax: cmake_dependent_option(<option> "<help_text>" <value> <depends> <force>)
cmake_dependent_option(NGEN_WITH_ROUTING "Build with t-route integration" ON "NGEN_WITH_PYTHON" OFF)
cmake_dependent_option(NGEN_UPDATE_GIT_SUBMODULES "Update submodules on configure" ON "GIT_FOUND;NGEN_HAS_GIT_DIR" OFF)

option(BMI_FORTRAN_ISO_C_LIB_DIR "Directory hint for middleware Fortran shared lib handling iso_c_binding" "${NGEN_ROOT_DIR}/extern/iso_c_fortran_bmi/cmake_build")
option(BMI_FORTRAN_ISO_C_LIB_NAME "Name for middleware Fortran shared lib handling iso_c_binding" "iso_c_bmi")

include(DeprecatedOptions)
deprecated_option(OLD MPI_ACTIVE            NEW NGEN_WITH_MPI     INCLUDE_ENV)
deprecated_option(OLD UDUNITS_ACTIVE        NEW NGEN_WITH_UDUNITS INCLUDE_ENV)
deprecated_option(OLD NGEN_ACTIVATE_PYTHON  NEW NGEN_WITH_PYTHON  INCLUDE_ENV)
deprecated_option(OLD NGEN_ACTIVATE_ROUTING NEW NGEN_WITH_ROUTING)
deprecated_option(OLD NETCDF_ACTIVE         NEW NGEN_WITH_NETCDF)
deprecated_option(OLD BMI_C_LIB_ACTIVE      NEW NGEN_WITH_BMI_C)
deprecated_option(OLD BMI_FORTRAN_ACTIVE    NEW NGEN_WITH_BMI_FORTRAN)
deprecated_option(OLD QUIET                 NEW NGEN_QUIET)
deprecated_option(OLD PACKAGE_TESTS         NEW NGEN_WITH_TESTS)
# Project =====================================================================

# Define project version and use via generated config header
project(ngen VERSION 0.1.0)

configure_file("${NGEN_INC_DIR}/NGenConfig.h.in" "${NGEN_INC_DIR}/NGenConfig.h")

add_executable(ngen "${NGEN_SRC_DIR}/NGen.cpp")

# Dependencies ================================================================

# -----------------------------------------------------------------------------
# Set global C++ options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# -----------------------------------------------------------------------------
# Check if NGen is the main project (e.g. development environment)
if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
    set(NGEN_IS_MAIN_PROJECT ON)
else()
    set(NGEN_IS_MAIN_PROJECT OFF)
endif()

# -----------------------------------------------------------------------------
# If MPI support is enabled, set the C++ compiler to "mpicxx"
if(NGEN_WITH_MPI)
    find_package(MPI REQUIRED)
    add_compile_definitions(NGEN_MPI_ACTIVE)
endif()

# -----------------------------------------------------------------------------
# FIXME: Is this needed for icpx? or even icpp? This only enables support for SYCL - Justin
# https://www.intel.com/content/www/us/en/develop/documentation/oneapi-dpcpp-cpp-compiler-dev-guide-and-reference/top/compiler-setup/use-the-command-line/use-cmake-with-the-compiler.html
if(INTEL_DPCPP)
    find_package(IntelDPCPP REQUIRED)
endif()

# -----------------------------------------------------------------------------
# Account for different OS and how that impacts shared lib file names
# FIXME: What is this used for? - Justin
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(SHARED_LIB_EXTENSION "dylib")
elseif(WIN32)
    error("Windows platforms are not currently supported")
else()
    set(SHARED_LIB_EXTENSION "so")
endif()

# -----------------------------------------------------------------------------
# Find the Boost library and configure usage
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
if(NOT DEFINED BOOST_ROOT AND NOT DEFINED ENV{BOOST_ROOT})
    # Look for version-specific Boost directory if available (known from Github Actions VM docs)
    if(DEFINED ENV{BOOST_ROOT_1_72_0})
        set(BOOST_ROOT $ENV{BOOST_ROOT_1_72_0})
    endif()
endif()
find_package(Boost 1.72.0 REQUIRED)

# -----------------------------------------------------------------------------
if(NGEN_WITH_SQLITE)
    find_package(SQLite3 REQUIRED)
    add_compile_definitions(NGEN_WITH_SQLITE3)
endif()

# -----------------------------------------------------------------------------
if(NGEN_WITH_UDUNITS)
    find_package(UDUNITS2 REQUIRED)
    add_compile_definitions(NGEN_WITH_UDUNITS)

    # Create UDUNITS2 target
    # Note: GLOBAL is required here in order to extend the scope of the target.
    #       see: https://stackoverflow.com/a/46491758/6891484
    add_library(libudunits2 SHARED IMPORTED GLOBAL)
    set_target_properties(libudunits2 PROPERTIES IMPORTED_LOCATION "${UDUNITS2_LIBRARY}")
    target_include_directories(libudunits2 INTERFACE "${UDUNITS2_INCLUDE}")
        
    get_target_property(_SUMMARY_UDUNITS_INCLUDE libudunits2 INTERFACE_INCLUDE_DIRECTORIES)
endif()

# -----------------------------------------------------------------------------
if(NGEN_WITH_NETCDF)
    set(NETCDF_CXX "YES")
    set(NETCDF_F77 "NO")
    set(NETCDF_F90 "NO")
    find_package(NetCDF REQUIRED)
    add_compile_definitions(NETCDF_ACTIVE)
    
    # Create NetCDF target
    add_library(NetCDF SHARED IMPORTED GLOBAL)
    target_include_directories(NetCDF INTERFACE "${NETCDF_INCLUDE_DIR}" "${NETCDF_CXX_INCLUDE_DIR}")
    set_target_properties(NetCDF
        PROPERTIES
            IMPORTED_LOCATION "${NETCDF_LIBRARY}"
            IMPORTED_LOCATION "${NETCDF_CXX_LIBRARY}")
endif()

# -----------------------------------------------------------------------------
# Handle several steps for BMI C library logic and dependencies, at top level, if functionality is turned on
if(NGEN_WITH_BMI_C)
    # Define associated preprocessor directive
    add_compile_definitions(NGEN_BMI_C_LIB_ACTIVE)
endif()

# -----------------------------------------------------------------------------
# Configure whether Fortran BMI functionality is active
if(NGEN_WITH_BMI_FORTRAN)
    add_compile_definitions(NGEN_BMI_FORTRAN_ACTIVE)

    # Searches for ISO C BMI library, defaults to
    # known development directories in the event the BMI_FORTRAN_ISO_C_*
    # variables are not set.
    find_library(BMI_FORTRAN_ISO_C_LIB_PATH
        NAMES
            "${BMI_FORTRAN_ISO_C_LIB_NAME}"
            "iso_c_bmi"
        HINTS # Searched before paths
            "${BMI_FORTRAN_ISO_C_LIB_DIR}" 
        PATHS
            ENV BMI_FORTRAN_ISO_C_LIB_DIR
            "${NGEN_ROOT_DIR}/extern/iso_c_fortran_bmi/cmake_build"
            "${NGEN_ROOT_DIR}/extern/iso_c_fortran_bmi/build"
        DOC "Path to middleware Fortran shared lib handling iso_c_binding"
    )

    if("${BMI_FORTRAN_ISO_C_LIB_PATH}" STREQUAL "BMI_FORTRAN_ISO_C_LIB_PATH-NOTFOUND")
        message(FATAL_ERROR "Failed to find BMI_FORTRAN_ISO_C_LIB_PATH:\n  BMI_FORTRAN_ISO_C_LIB_NAME: ${BMI_FORTRAN_ISO_C_LIB_NAME}\n  BMI_FORTRAN_ISO_C_LIB_DIR: ${BMI_FORTRAN_ISO_C_LIB_DIR}")
    endif()
    
    add_library(bmi_fortran_compat SHARED IMPORTED)
    set_target_properties(bmi_fortran_compat PROPERTIES IMPORTED_LOCATION "${BMI_FORTRAN_ISO_C_LIB_PATH}")
endif()

# -----------------------------------------------------------------------------
if(NGEN_WITH_PYTHON)
    add_compile_definitions(ACTIVATE_PYTHON=true)
    find_package(Python 3.6.8 REQUIRED COMPONENTS Interpreter Development NumPy)
    set(PYTHON_EXECUTABLE ${Python_EXECUTABLE}) # Case-sensitive difference
    add_subdirectory(extern/pybind11)
endif()

# -----------------------------------------------------------------------------
if(NGEN_WITH_ROUTING)
    add_compile_definitions(NGEN_ROUTING_ACTIVE)
endif()

# -----------------------------------------------------------------------------
if(NGEN_QUIET)
    set(UDUNITS_QUIET true)
    set(ET_QUIET true)
    add_compile_definitions(NGEN_QUIET)
endif()

if(UDUNITS_QUIET)
    add_compile_definitions(UDUNITS_QUIET)
endif()

# FIXME: I don't think this is needed now? - Justin
if(ET_QUIET)
    add_compile_definitions(ET_QUIET)
endif()

# -----------------------------------------------------------------------------

# Natively support BMI C++ modules and pre-compile in the test_bmi_cpp mock/example.
set(TEST_BMI_CPP_DIR ${NGEN_EXT_DIR}/test_bmi_cpp)
git_update_submodule(${NGEN_EXT_DIR}/bmi-cxx)
add_subdirectory(${TEST_BMI_CPP_DIR} ${TEST_BMI_CPP_DIR}/cmake_build) # FIXME: Why is this here? - Justin

# -----------------------------------------------------------------------------
# Project Targets
# -----------------------------------------------------------------------------
add_dependencies(ngen testbmicppmodel)
target_include_directories(ngen PUBLIC "${NGEN_INC_DIR}")

add_subdirectory("src/core")
add_subdirectory("src/geojson")
add_subdirectory("src/geopackage")
add_subdirectory("src/realizations/catchment")
add_subdirectory("src/forcing")
add_subdirectory("src/utilities/mdarray")
add_subdirectory("src/utilities/mdframe")

target_link_libraries(ngen
    PUBLIC
        NGen::core
        NGen::core_catchment
        NGen::core_nexus
        NGen::geojson
        NGen::geopackage
        NGen::realizations_catchment
        NGen::forcing
        NGen::core_mediator
)

if(NGEN_WITH_ROUTING)
    add_subdirectory("src/routing")
    target_link_libraries(ngen PUBLIC NGen::routing)
endif()

add_executable(partitionGenerator src/partitionGenerator.cpp)
target_include_directories(partitionGenerator PUBLIC "${PROJECT_BINARY_DIR}/include")
target_link_libraries(partitionGenerator PUBLIC NGen::core NGen::geojson)

# For automated testing with Google Test
if(NGEN_WITH_TESTS)
    include(CTest) # calls enable_testing()
    include(GoogleTest)
    add_subdirectory(test)
endif()

# -----------------------------------------------------------------------------
# Build Summary
# -----------------------------------------------------------------------------

# Syntax: ngen_multiline_message("<message 1>" "<message 2>" ...)
function(ngen_multiline_message)
set(messages "${ARGN}")
foreach(msg IN LISTS messages)
    message(STATUS "${msg}")
endforeach()
endfunction()

# Syntax: ngen_dependent_multiline_message(<variable> "<message 1>" "<message 2>" ...)
function(ngen_dependent_multiline_message VARIABLE)
if(${VARIABLE})
    ngen_multiline_message(${ARGN})
endif()
endfunction()

# This is used purely for configuration output,
# and does not affect the virtual environment used.
if(DEFINED ENV{VIRTUAL_ENV})
    set(NGEN_CONFIGURED_VENV "$ENV{VIRTUAL_ENV}")
else()
    set(NGEN_CONFIGURED_VENV "<none>")
endif()

# MPI CXX libraries output both C and CXX libs
string(REPLACE ";" "," _MPI_CXX_LIBS "${MPI_CXX_LIBRARIES}")

message(STATUS "---------------------------------------------------------------------")
ngen_multiline_message(
"NGen version: ${ngen_VERSION}"
"Build configuration summary:"
"  Generator: ${CMAKE_GENERATOR}"
"  Build type: ${CMAKE_BUILD_TYPE}"
"  System: ${CMAKE_SYSTEM_NAME}"
"  C Compiler: ${CMAKE_C_COMPILER}"
"  CXX Compiler: ${CMAKE_CXX_COMPILER}"
"  Flags:"
"    NGEN_WITH_MPI: ${NGEN_WITH_MPI}"
"    NGEN_WITH_NETCDF: ${NGEN_WITH_NETCDF}"
"    NGEN_WITH_SQLITE: ${NGEN_WITH_SQLITE}"
"    NGEN_WITH_UDUNITS: ${NGEN_WITH_UDUNITS}"
"    NGEN_WITH_BMI_FORTRAN: ${NGEN_WITH_BMI_FORTRAN}"
"    NGEN_WITH_BMI_C: ${NGEN_WITH_BMI_C}"
"    NGEN_WITH_PYTHON: ${NGEN_WITH_PYTHON}"
"    NGEN_WITH_ROUTING: ${NGEN_WITH_ROUTING}"
"    NGEN_WITH_TESTS: ${NGEN_WITH_TESTS}"
"    NGEN_QUIET: ${NGEN_QUIET}"
"Environment summary:"
"  Boost:"
"    Version: ${Boost_VERSION}"
"    Include: ${Boost_INCLUDE_DIRS}")
ngen_dependent_multiline_message(INTEL_DPCPP
"  Intel DPC++:"
"    Version: ${SYCL_LANGUAGE_VERSION}"
"    Include: ${SYCL_INCLUDE_DIR}"
"    Implmentation: ${SYCL_IMPLEMENTATION_ID}")
ngen_dependent_multiline_message(NGEN_WITH_MPI
"  MPI (C):"
"    Version: ${MPI_C_VERSION}"
"    Library: ${MPI_C_LIBRARIES}"
"    Include: ${MPI_C_INCLUDE_DIRS}")
ngen_dependent_multiline_message(NGEN_WITH_MPI
"  MPI (CXX):"
"    Version: ${MPI_CXX_VERSION}"
"    Library: ${_MPI_CXX_LIBS}"
"    Include: ${MPI_CXX_INCLUDE_DIRS}")
ngen_dependent_multiline_message(NGEN_WITH_NETCDF
"  NetCDF:"
"    Library: ${NETCDF_LIBRARY}"
"    Library (CXX): ${NETCDF_CXX_LIBRARIES}"
"    Include: ${NETCDF_INCLUDE_DIR}"
"    Parallel: ${NETCDF_HAS_PARALLEL}")
ngen_dependent_multiline_message(NGEN_WITH_SQLITE
"  SQLite:"
"    Version: ${SQLite3_VERSION}"
"    Library: ${SQLite3_LIBRARIES}"
"    Include: ${SQLite3_INCLUDE_DIRS}")
ngen_dependent_multiline_message(NGEN_WITH_UDUNITS
"  UDUNITS2:"
"    Library: ${UDUNITS2_LIBRARY}"
"    Include: ${_SUMMARY_UDUNITS_INCLUDE}")
ngen_dependent_multiline_message(NGEN_WITH_BMI_FORTRAN
"  Fortran:"
"    BMI_FORTRAN_ISO_C_LIB_PATH: ${BMI_FORTRAN_ISO_C_LIB_PATH}"
"    BMI_FORTRAN_ISO_C_LIB_NAME: ${BMI_FORTRAN_ISO_C_LIB_NAME}"
"    BMI_FORTRAN_ISO_C_LIB_DIR: ${BMI_FORTRAN_ISO_C_LIB_DIR}")
ngen_dependent_multiline_message(NGEN_WITH_PYTHON
"  Python:"
"    Version: ${Python_VERSION}"
"    Virtual Env: ${NGEN_CONFIGURED_VENV}"
"    Executable: ${Python_EXECUTABLE}"
"    Interpreter Type: ${Python_INTERPRETER_ID}"
"    Site Library: ${Python_SITELIB}"
"    Include: ${Python_INCLUDE_DIRS}"
"    Runtime Library: ${Python_RUNTIME_LIBRARY_DIRS}"
"    NumPy Version: ${Python_NumPy_VERSION}"
"    NumPy Include: ${Python_NumPy_INCLUDE_DIRS}"
"    pybind11 Version: ${pybind11_VERSION}"
"    pybind11 Include: ${pybind11_INCLUDE_DIR}")
message(STATUS "---------------------------------------------------------------------")

#set_target_properties(ngen PROPERTIES LINKER_LANGUAGE CXX)
#
#target_compile_options(ngen PUBLIC -std=c++14 -Wall)
#target_compile_features(ngen PUBLIC cxx_std_14)
#add_subdirectory("src/geojson")
