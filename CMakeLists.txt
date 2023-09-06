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
list(APPEND CMAKE_MODULE_PATH "${NGEN_MOD_DIR}/modules")

# Provides:
#   (1) finds Git package
#   (2) Sets ${NGEN_HAS_GIT_DIR} to ON/OFF if ${NGEN_ROOT_DIR}/.git exists
#   (3) Function `git_update_submodule` which updates a given submodule
include(GitUpdateSubmodules)

# Options =====================================================================
include(CMakeDependentOption)

option(NGEN_WITH_MPI         "Build with MPI support"             OFF)
option(NGEN_WITH_NETCDF      "Build with NetCDF support"          ON)
option(NGEN_WITH_SQLITE      "Build with SQLite3 support"         ON)
option(NGEN_WITH_UDUNITS     "Build with UDUNITS2 support"        ON)
option(NGEN_WITH_BMI_FORTRAN "Build with Fortran BMI support"     ON)
option(NGEN_WITH_BMI_C       "Build with C BMI support"           ON)
option(NGEN_WITH_PYTHON      "Build with embedded Python support" ON)
option(NGEN_WITH_TESTS       "Build with unit tests"              ON)

# These options require dependency on some of the above options
# Syntax: cmake_dependent_option(<option> "<help_text>" <value> <depends> <force>)
cmake_dependent_option(NGEN_WITH_ROUTING "Build with t-route integration" OFF "NGEN_WITH_PYTHON" OFF)
cmake_dependent_option(NGEN_ISO_C_FORTRAN_BMI_LIB_PATH "Path to iso_c_fortran_bmi library" "" "NGEN_WITH_BMI_FORTRAN" "")
cmake_dependent_option(NGEN_UPDATE_GIT_SUBMODULES "Update submodules on configure" ON "GIT_FOUND;NGEN_HAS_GIT_DIR" OFF)
# Project =====================================================================

# Define project version and use via generated config header
project(ngen VERSION 0.1.0)

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
    find_package(MPI REQUIRED QUIET)
    add_compile_definitions(NGEN_MPI_ACTIVE)
endif()

# -----------------------------------------------------------------------------
# FIXME: Is this needed for icpx? or even icpp? This only enables support for SYCL - Justin
# https://www.intel.com/content/www/us/en/develop/documentation/oneapi-dpcpp-cpp-compiler-dev-guide-and-reference/top/compiler-setup/use-the-command-line/use-cmake-with-the-compiler.html
if(INTEL_DPCPP)
    find_package(IntelDPCPP REQUIRED QUIET)
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

    add_library(udunits2 SHARED IMPORTED)
    target_include_directories(udunits2 INTERFACE "${UDUNITS2_INCLUDE}")
    set_target_properties(udunits2 PROPERTIES IMPORTED_LOCATION "${UDUNITS2_LIBRARY}")
endif()

# -----------------------------------------------------------------------------
if(NETCDF_ACTIVE)
    set(NETCDF_CXX "YES")
    set(NETCDF_F77 "NO")
    set(NETCDF_F90 "NO")
    find_package(NetCDF REQUIRED)
    add_compile_definitions(NETCDF_ACTIVE)
    
    add_library(NetCDF SHARED IMPORTED)
    target_include_directories(NetCDF INTERFACE "${NETCDF_INCLUDE_DIR}" "${NETCDF_CXX_INCLUDE_DIR}")
    set_target_properties(NetCDF PROPERTIES IMPORTED_LOCATION "${NETCDF_LIBRARIES};${NETCDF_CXX_LIBRARY}")
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
endif()

# -----------------------------------------------------------------------------
if(NGEN_WITH_PYTHON)
    add_compile_definitions(ACTIVATE_PYTHON=true)
    find_package(Python 3.6.8 REQUIRED COMPONENTS Interpreter Development NumPy)
    set(PYTHON_EXECUTABLE ${Python_EXECUTABLE}) # Case-sensitive difference
    add_subdirectory(extern/pybind11)
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


configure_file(include/NGenConfig.h.in include/NGenConfig.h)
add_executable(ngen "${NGEN_SRC_DIR}/NGen.cpp")
add_dependencies(ngen testbmicppmodel)
target_include_directories(ngen PUBLIC "${PROJECT_BINARY_DIR}/include")

add_subdirectory("src/core")
add_subdirectory("src/geojson")

if(NGEN_WITH_SQLITE3)
    add_subdirectory("src/geopackage")
    target_link_libraries(ngen PUBLIC NGen::geopackage)
endif()

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
        NGen::realizations_catchment
        NGen::forcing
        NGen::core_mediator
        udunits2
        NetCDF
)

add_executable(partitionGenerator src/partitionGenerator.cpp)
target_include_directories(partitionGenerator PUBLIC "${PROJECT_BINARY_DIR}/include")
target_link_libraries(partitionGenerator PUBLIC NGen::core NGen::geojson)

if(NGEN_ACTIVATE_ROUTING)
    add_compile_definitions(NGEN_ROUTING_ACTIVE)
    add_subdirectory("src/routing")
    target_link_libraries(ngen PUBLIC NGen::routing)
endif()

# For automated testing with Google Test
if(PACKAGE_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(test)
endif()

# -----------------------------------------------------------------------------
# Build Summary
# -----------------------------------------------------------------------------

macro(message_if_defined PRINTNAME)
if(${ARGN})
message(STATUS "    ${PRINTNAME}: ${${ARGN}}")
else()
message(STATUS "    ${PRINTNAME}: OFF")
endif()
endmacro()

message("")
message(STATUS "---------------------------------------------------------------------")
message(STATUS "NGen version: ${ngen_VERSION}")
message(STATUS)
message(STATUS "Build configuration summary:")
message(STATUS "  Generator: ${CMAKE_GENERATOR}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  CXX Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  Flags:")

message(STATUS "    NGEN_WITH_MPI: ${NGEN_WITH_MPI}")
message(STATUS "    NGEN_WITH_NETCDF: ${NGEN_WITH_NETCDF}")
message(STATUS "    NGEN_WITH_SQLITE: ${NGEN_WITH_SQLITE}")
message(STATUS "    NGEN_WITH_UDUNITS: ${NGEN_WITH_UDUNITS}")
message(STATUS "    NGEN_WITH_BMI_FORTRAN: ${NGEN_WITH_BMI_FORTRAN}")
message(STATUS "    NGEN_WITH_BMI_C: ${NGEN_WITH_BMI_C}")
message(STATUS "    NGEN_WITH_PYTHON: ${NGEN_WITH_PYTHON}")
message(STATUS "    NGEN_WITH_ROUTING: ${NGEN_WITH_ROUTING}")
message(STATUS "    NGEN_WITH_TESTS: ${NGEN_WITH_TESTS}")
message(STATUS "    NGEN_QUIET: ${NGEN_QUIET}")

message(STATUS)

message(STATUS "Environment summary:")
message(STATUS)

message(STATUS "  Boost:")
message(STATUS "    Version: ${Boost_VERSION}")
message(STATUS "    Include: ${Boost_INCLUDE_DIRS}")
message(STATUS)

if(INTEL_DPCPP)
message(STATUS "  Intel DPC++:")
message(STATUS "    Version: ${SYCL_LANGUAGE_VERSION}")
message(STATUS "    Include: ${SYCL_INCLUDE_DIR}")
message(STATUS "    Implmentation: ${SYCL_IMPLEMENTATION_ID}")
message(STATUS)
endif()

if(NGEN_WITH_MPI)
message(STATUS "  MPI (CXX):")
message(STATUS "    Version: ${MPI_CXX_VERSION}")
message(STATUS "    Library: ${MPI_CXX_LIBRARIES}")
message(STATUS "    Include: ${MPI_CXX_INCLUDE_DIRS}")
message(STATUS)
endif()

if(NGEN_WITH_NETCDF)
message(STATUS "  NetCDF:")
message(STATUS "    Library: ${NETCDF_LIBRARY}")
message(STATUS "    Library (CXX): ${NETCDF_CXX_LIBRARIES}")
message(STATUS "    Include: ${NETCDF_INCLUDE_DIR}")
message(STATUS "    Parallel: ${NETCDF_HAS_PARALLEL}")
message(STATUS)
endif()

if(NGEN_WITH_SQLITE)
message(STATUS "  SQLite:")
message(STATUS "    Version: ${SQLite3_VERSION}")
message(STATUS "    Library: ${SQLite3_LIBRARIES}")
message(STATUS "    Include: ${SQLite3_INCLUDE_DIRS}")
message(STATUS)
endif()

if(NGEN_WITH_UDUNITS)
message(STATUS "  UDUNITS2:")
message(STATUS "    Library: ${UDUNITS2_LIBRARY}")
message(STATUS "    Include: ${UDUNITS2_INCLUDE}")
message(STATUS)
endif()

if(NGEN_WITH_BMI_FORTRAN)
message(STATUS "  Fortran:")
message(STATUS "    Fortran-C BMI Compat Library Path: ${NGEN_ISO_C_FORTRAN_BMI_LIB_PATH}")
message(STATUS)
endif()

if(NGEN_WITH_PYTHON)
    if(DEFINED ENV{VIRTUAL_ENV})
        set(NGEN_CONFIGURED_VENV "$ENV{VIRTUAL_ENV}")
    else()
        set(NGEN_CONFIGURED_VENV "<none>")
    endif()
    message(STATUS "  Python:")
    message(STATUS "    Version: ${Python_VERSION}")
    message(STATUS "    Virtual Env: ${NGEN_CONFIGURED_VENV}")
    message(STATUS "    Executable: ${Python_EXECUTABLE}")
    message(STATUS "    Interpreter Type: ${Python_INTERPRETER_ID}")
    message(STATUS "    Site Library: ${Python_SITELIB}")
    message(STATUS "    Include: ${Python_INCLUDE_DIRS}")
    message(STATUS "    Runtime Library: ${Python_RUNTIME_LIBRARY_DIRS}")
    message(STATUS "    NumPy Version: ${Python_NumPy_VERSION}")
    message(STATUS "    NumPy Include: ${Python_NumPy_INCLUDE_DIRS}")
    if(pybind11_FOUND)
        message(STATUS "    pybind11:")
        message(STATUS "      Version: ${pybind11_VERSION}")
        message(STATUS "      Include: ${pybind11_INCLUDE_DIR}")
    endif()
endif()
message(STATUS "---------------------------------------------------------------------")

#set_target_properties(ngen PROPERTIES LINKER_LANGUAGE CXX)
#
#target_compile_options(ngen PUBLIC -std=c++14 -Wall)
#target_compile_features(ngen PUBLIC cxx_std_14)
#add_subdirectory("src/geojson")
