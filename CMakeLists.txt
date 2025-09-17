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

# find_package() uses upper-case <PACKAGENAME>_ROOT variables in conjunction with CMP0074.
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
#   (4) Sets ${NGEN_GIT_COMMIT} to the current commit hash if available
include(GitUpdateSubmodules)

# NGen CMake module for automating external subdirectory builds (i.e. testbmicpp)
# Provides:
#   (1) add_external_subdirectory(<path>)
include(ExternalSubdirectory)

# Options =====================================================================
include(CMakeDependentOption)

option(NGEN_WITH_MPI         "Build with MPI support"             OFF)
option(NGEN_WITH_NETCDF      "Build with NetCDF support"          OFF)
option(NGEN_WITH_SQLITE      "Build with SQLite3 support"         OFF)
option(NGEN_WITH_UDUNITS     "Build with UDUNITS2 support"        ON)
option(NGEN_WITH_BMI_FORTRAN "Build with Fortran BMI support"     OFF)
option(NGEN_WITH_BMI_C       "Build with C BMI support"           ON)
option(NGEN_WITH_PYTHON      "Build with embedded Python support" ON)
option(NGEN_WITH_TESTS       "Build with unit tests"              ON)
option(NGEN_QUIET            "Silence output"                     OFF)

# These options require dependency on some of the above options
# Syntax: cmake_dependent_option(<option> "<help_text>" <value> <depends> <force>)
cmake_dependent_option(NGEN_WITH_ROUTING "Build with t-route integration" ON "NGEN_WITH_PYTHON" OFF)
cmake_dependent_option(NGEN_UPDATE_GIT_SUBMODULES "Update submodules on configure" ON "GIT_FOUND;NGEN_HAS_GIT_DIR" OFF)
cmake_dependent_option(NGEN_WITH_COVERAGE "Build with test coverage" OFF "NGEN_WITH_TESTS" OFF)

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

# Options to automate building of extern-distributed models
option(NGEN_WITH_EXTERN_SLOTH                            "Build with extern-distributed SLoTH"            ON)
option(NGEN_WITH_EXTERN_SMP                              "Build with extern-distributed SMP"              OFF)
option(NGEN_WITH_EXTERN_SFT                              "Build with extern-distributed SFT"              OFF)
cmake_dependent_option(NGEN_WITH_EXTERN_TOPMODEL         "Build with extern-distributed TOPMODEL"         ON "NGEN_WITH_BMI_C"       OFF)
cmake_dependent_option(NGEN_WITH_EXTERN_CFE              "Build with extern-distributed CFE"              ON "NGEN_WITH_BMI_C"       OFF)
cmake_dependent_option(NGEN_WITH_EXTERN_LGAR             "Build with extern-distributed LGAR"             ON "NGEN_WITH_BMI_C"       OFF)
cmake_dependent_option(NGEN_WITH_EXTERN_PET              "Build with extern-distributed PET"              ON "NGEN_WITH_BMI_C"       OFF)
cmake_dependent_option(NGEN_WITH_EXTERN_NOAH_OWP_MODULAR "Build with extern-distributed Noah-OWP-Modular" ON "NGEN_WITH_BMI_FORTRAN" OFF)

# Allow enabling of all extern models dependent on NGen options given.
option(NGEN_WITH_EXTERN_ALL OFF)
if(NGEN_WITH_EXTERN_ALL)
    set(NGEN_WITH_EXTERN_SLOTH ON)
    set(NGEN_WITH_EXTERN_SMP   ON)
    set(NGEN_WITH_EXTERN_SFT   ON)

    # Ensure C support is enabled
    if(NGEN_WITH_BMI_C)
        set(NGEN_WITH_EXTERN_TOPMODEL ON)
        set(NGEN_WITH_EXTERN_CFE ON)
        set(NGEN_WITH_EXTERN_PET ON)
        set(NGEN_WITH_EXTERN_LGAR ON)
    else()
        message(FATAL_ERROR "NGEN_WITH_EXTERN_ALL set to ${NGEN_WITH_EXTERN_ALL}, but NGEN_WITH_BMI_C set to ${NGEN_WITH_BMI_C}")
    endif()

    # Ensure Fortran support is enabled
    if(NGEN_WITH_BMI_FORTRAN)
        set(NGEN_WITH_EXTERN_NOAH_OWP_MODULAR ON)
    else()
        message(FATAL_ERROR "NGEN_WITH_EXTERN_ALL set to ${NGEN_WITH_EXTERN_ALL}, but NGEN_WITH_BMI_FORTRAN set to ${NGEN_WITH_BMI_FORTRAN}")
    endif()

    # Ensure Python support is enabled
    if(NOT NGEN_WITH_PYTHON)
        message(FATAL_ERROR "NGEN_WITH_EXTERN_ALL set to ${NGEN_WITH_EXTERN_ALL}, but NGEN_WITH_PYTHON set to ${NGEN_WITH_PYTHON}")
    endif()
endif()

# Project =====================================================================

# -----------------------------------------------------------------------------
# Set global C++ options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Define project version and use via generated config header
project(ngen VERSION 0.3.0)

add_executable(ngen "${NGEN_SRC_DIR}/NGen.cpp")

# Dependencies ================================================================

# -----------------------------------------------------------------------------
# Check if NGen is the main project (e.g. development environment)
if(PROJECT_NAME STREQUAL CMAKE_PROJECT_NAME)
    set(NGEN_IS_MAIN_PROJECT ON)
else()
    set(NGEN_IS_MAIN_PROJECT OFF)
endif()

# -----------------------------------------------------------------------------
# If Coverage is enabled, include coverage module and
# set compiler flags/linkage to accomodate coverage.
if(NGEN_WITH_COVERAGE)
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif()

# -----------------------------------------------------------------------------
# If MPI support is enabled, set the C++ compiler to "mpicxx"
if(NGEN_WITH_MPI)
    find_package(MPI REQUIRED)
endif()

# -----------------------------------------------------------------------------
# TODO: Used for compatibility on Hera. Refactor this in the future.
# https://www.intel.com/content/www/us/en/develop/documentation/oneapi-dpcpp-cpp-compiler-dev-guide-and-reference/top/compiler-setup/use-the-command-line/use-cmake-with-the-compiler.html
if(INTEL_DPCPP)
    find_package(IntelDPCPP REQUIRED)
endif()

# -----------------------------------------------------------------------------
# Account for different OS and how that impacts shared lib file names
if(WIN32)
    message(FATAL_ERROR "Windows platforms are not currently supported")
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(NGEN_SHARED_LIB_EXTENSION "dylib")
else()
    set(NGEN_SHARED_LIB_EXTENSION "so")
endif()
add_compile_definitions(NGEN_SHARED_LIB_EXTENSION)

# -----------------------------------------------------------------------------
# Find the Boost library and configure usage
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.79.0 REQUIRED)

# -----------------------------------------------------------------------------
if(NGEN_WITH_SQLITE)
    find_package(SQLite3 REQUIRED)
    add_compile_definitions(NGEN_WITH_SQLITE3)
endif()

# -----------------------------------------------------------------------------
if(NGEN_WITH_UDUNITS)
    find_package(UDUNITS2 REQUIRED)
    add_compile_definitions(NGEN_WITH_UDUNITS)
endif()

# -----------------------------------------------------------------------------
if(NGEN_WITH_NETCDF)
    find_package(NetCDF COMPONENTS CXX)

    if(NOT NetCDF_CXX_FOUND)
        find_package(NetCDF REQUIRED)
        message(INFO " Using internal copy of netcdf-cxx4")
        add_external_subdirectory(
            SOURCE "${NGEN_EXT_DIR}/netcdf-cxx4"
            OUTPUT "${PROJECT_BINARY_DIR}/extern-libs/netcdf-cxx4"
            GIT_UPDATE "${NGEN_EXT_DIR}/netcdf-cxx4/netcdf-cxx4"
	    IMPORTS netcdf-cxx4
        )
        set(NetCDF_CXX_INCLUDE_DIR "${NGEN_EXT_DIR}/netcdf-cxx4/netcdf-cxx4/cxx4")
        set(NetCDF_CXX_LIBRARY "${PROJECT_BINARY_DIR}/extern-libs/netcdf-cxx4/libnetcdf-cxx4.a")
        target_link_libraries(NetCDF INTERFACE netcdf-cxx4)
    endif()
endif()

if(NGEN_WITH_EXTERN_SLOTH)
    # This is set because SLOTH will link to gtest by default,
    # which causes a configure error.
    set(PACKAGE_TESTS OFF CACHE BOOL "SLoTH testing")
    add_external_subdirectory(
        SOURCE     "${NGEN_EXT_DIR}/sloth"
        OUTPUT     "${NGEN_EXT_DIR}/sloth/cmake_build"
        GIT_UPDATE "${NGEN_EXT_DIR}/sloth"
        IMPORTS    slothmodel
    )
    set_target_properties(slothmodel PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

if (NGEN_WITH_EXTERN_SMP OR NGEN_WITH_EXTERN_SFT)
    # SoilMoistureProfiles and SoilFreezeThaw expect this setting,
    # as they have alternative standalone build modes.
    set(NGEN ON CACHE BOOL "Type setting for SMP and SFT")
endif()

if(NGEN_WITH_EXTERN_SMP)
    # SoilMoistureProfiles
    add_external_subdirectory(
        SOURCE     "${NGEN_EXT_DIR}/SoilMoistureProfiles/SoilMoistureProfiles"
        OUTPUT     "${NGEN_EXT_DIR}/SoilMoistureProfiles/cmake_build"
        GIT_UPDATE "${NGEN_EXT_DIR}/SoilMoistureProfiles/SoilMoistureProfiles"
        IMPORTS    smpbmi
    )
endif()

if(NGEN_WITH_EXTERN_SFT)
    # SoilFreezeThaw
    add_external_subdirectory(
        SOURCE     "${NGEN_EXT_DIR}/SoilFreezeThaw/SoilFreezeThaw"
        OUTPUT     "${NGEN_EXT_DIR}/SoilFreezeThaw/cmake_build"
        GIT_UPDATE "${NGEN_EXT_DIR}/SoilFreezeThaw/SoilFreezeThaw"
        IMPORTS    sftbmi
    )
endif()

# -----------------------------------------------------------------------------
# Handle several steps for BMI C library logic and dependencies, at top level, if functionality is turned on
if(NGEN_WITH_BMI_C)
    if(NGEN_WITH_EXTERN_TOPMODEL)
        add_external_subdirectory(
            SOURCE     "${NGEN_EXT_DIR}/topmodel"
            OUTPUT     "${NGEN_EXT_DIR}/topmodel/cmake_build"
            GIT_UPDATE "${NGEN_EXT_DIR}/topmodel/topmodel"
            IMPORTS    topmodelbmi
        )
    endif()

    if(NGEN_WITH_EXTERN_CFE)
        add_external_subdirectory(
            SOURCE     "${NGEN_EXT_DIR}/cfe"
            OUTPUT     "${NGEN_EXT_DIR}/cfe/cmake_build"
            GIT_UPDATE "${NGEN_EXT_DIR}/cfe/cfe"
            IMPORTS    cfebmi
        )
    endif()

    if(NGEN_WITH_EXTERN_PET)
        add_external_subdirectory(
            SOURCE "${NGEN_EXT_DIR}/evapotranspiration/evapotranspiration"
            OUTPUT "${NGEN_EXT_DIR}/evapotranspiration/evapotranspiration/cmake_build"
            GIT_UPDATE "${NGEN_EXT_DIR}/evapotranspiration/evapotranspiration"
            IMPORTS petbmi
        )
    endif()

    if(NGEN_WITH_EXTERN_LGAR)
        add_external_subdirectory(
            SOURCE     "${NGEN_EXT_DIR}/lgar-c"
            OUTPUT     "${NGEN_EXT_DIR}/lgar-c/cmake_build"
            GIT_UPDATE "${NGEN_EXT_DIR}/lgar-c/LGAR-C"
            IMPORTS    lasambmi
        )
    endif()
endif()

# -----------------------------------------------------------------------------
# Configure whether Fortran BMI functionality is active
if(NGEN_WITH_BMI_FORTRAN)
    add_external_subdirectory(
        SOURCE  "${NGEN_EXT_DIR}/iso_c_fortran_bmi"
        OUTPUT  "${NGEN_EXT_DIR}/iso_c_fortran_bmi/cmake_build"
        IMPORTS iso_c_bmi
    )

    if(NGEN_WITH_EXTERN_NOAH_OWP_MODULAR)
        add_external_subdirectory(
            SOURCE     "${NGEN_EXT_DIR}/noah-owp-modular"
            OUTPUT     "${NGEN_EXT_DIR}/noah-owp-modular/cmake_build"
            GIT_UPDATE "${NGEN_EXT_DIR}/noah-owp-modular/noah-owp-modular"
            IMPORTS    surfacebmi
        )
    endif()
endif()

# -----------------------------------------------------------------------------
if(NGEN_WITH_PYTHON)
    find_package(Python 3.6.8 REQUIRED COMPONENTS Interpreter Development NumPy)
    if(${Python_NumPy_VERSION} VERSION_GREATER_EQUAL 2)
        message(FATAL_ERROR "Found numpy version \"${Python_NumPy_VERSION}\"; numpy>=2.0.0 is not currently supported")
    endif()
    set(PYTHON_EXECUTABLE ${Python_EXECUTABLE}) # Case-sensitive difference
    add_subdirectory(extern/pybind11 pybind11)
endif()

# -----------------------------------------------------------------------------
if(NGEN_QUIET)
    set(UDUNITS_QUIET true)
    add_compile_definitions(NGEN_QUIET)
endif()

if(UDUNITS_QUIET)
    add_compile_definitions(UDUNITS_QUIET)
endif()


# -----------------------------------------------------------------------------
# Project Targets
# -----------------------------------------------------------------------------

# Contains NGenConfig.h
add_library(ngen_config_header INTERFACE)
add_library(NGen::config_header ALIAS ngen_config_header)
target_include_directories(ngen_config_header INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/include")

if(NGEN_WITH_EXTERN_SLOTH)
    add_dependencies(ngen slothmodel)
endif()

if(NGEN_WITH_EXTERN_SMP)
    add_dependencies(ngen smpbmi)
endif()

if(NGEN_WITH_EXTERN_SFT)
    add_dependencies(ngen sftbmi)
endif()

if(NGEN_WITH_EXTERN_TOPMODEL)
    add_dependencies(ngen topmodelbmi)
endif()

if(NGEN_WITH_EXTERN_CFE)
    add_dependencies(ngen cfebmi)
endif()

if(NGEN_WITH_EXTERN_PET)
    add_dependencies(ngen petbmi)
endif()

if(NGEN_WITH_EXTERN_LGAR)
    add_dependencies(ngen lasambmi)
endif()

if(NGEN_WITH_EXTERN_NOAH_OWP_MODULAR)
    add_dependencies(ngen surfacebmi)
endif()

target_include_directories(ngen PUBLIC "${NGEN_INC_DIR}")

add_subdirectory("src/core")
add_subdirectory("src/geojson")
add_subdirectory("src/bmi")
add_subdirectory("src/realizations/catchment")
add_subdirectory("src/forcing")
add_subdirectory("src/utilities/mdarray")
add_subdirectory("src/utilities/mdframe")
add_subdirectory("src/utilities/logging")

target_link_libraries(ngen
    PUBLIC
        NGen::config_header
        NGen::core
        NGen::core_catchment
        NGen::core_nexus
        NGen::geojson
        NGen::realizations_catchment
        NGen::forcing
        NGen::core_mediator
        NGen::logging
)

if(NGEN_WITH_SQLITE)
    add_subdirectory("src/geopackage")
    target_link_libraries(ngen PUBLIC NGen::geopackage)
endif()

if(NGEN_WITH_ROUTING)
    add_subdirectory("src/routing")
    target_link_libraries(ngen PUBLIC NGen::routing)
endif()

add_executable(partitionGenerator src/partitionGenerator.cpp)
target_include_directories(partitionGenerator PUBLIC "${PROJECT_BINARY_DIR}/include")
if(NGEN_WITH_SQLITE)
    target_include_directories(partitionGenerator PUBLIC AFTER "${NGEN_INC_DIR}/geopackage")
endif()

target_link_libraries(partitionGenerator PUBLIC NGen::core NGen::geojson)
if(NGEN_WITH_SQLITE)
    target_link_libraries(partitionGenerator PUBLIC NGen::geopackage)
endif()

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
macro(ngen_multiline_message)
set(messages "${ARGN}")
foreach(msg IN LISTS messages)
    set(_CONF "${NGEN_CONF_SUMMARY}")
    string(APPEND _CONF "${msg}\n")
    set(NGEN_CONF_SUMMARY "${_CONF}")

    message(STATUS "${msg}")
endforeach()
endmacro()

# Syntax: ngen_dependent_multiline_message(<variable> "<message 1>" "<message 2>" ...)
macro(ngen_dependent_multiline_message VARIABLE)
if(${VARIABLE})
    ngen_multiline_message(${ARGN})
endif()
endmacro()

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
"  C Flags: ${CMAKE_C_FLAGS}"
"  CXX Compiler: ${CMAKE_CXX_COMPILER}"
"  CXX Flags: ${CMAKE_CXX_FLAGS}")
ngen_dependent_multiline_message(NGEN_HAS_GIT_DIR
"  Git Commit ID: ${NGEN_GIT_COMMIT}")
ngen_multiline_message(
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
"    NGEN_WITH_COVERAGE: ${NGEN_WITH_COVERAGE}"
"    NGEN_QUIET: ${NGEN_QUIET}"
"  Extern Models:"
"    NGEN_WITH_EXTERN_ALL: ${NGEN_WITH_EXTERN_ALL}"
"    NGEN_WITH_EXTERN_SLOTH: ${NGEN_WITH_EXTERN_SLOTH}"
"    NGEN_WITH_EXTERN_TOPMODEL: ${NGEN_WITH_EXTERN_TOPMODEL}"
"    NGEN_WITH_EXTERN_CFE: ${NGEN_WITH_EXTERN_CFE}"
"    NGEN_WITH_EXTERN_PET: ${NGEN_WITH_EXTERN_PET}"
"    NGEN_WITH_EXTERN_NOAH_OWP_MODULAR: ${NGEN_WITH_EXTERN_NOAH_OWP_MODULAR}"
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
"    Version: ${NetCDF_VERSION}"
"    Library: ${NetCDF_LIBRARY}"
"    Library (CXX): ${NetCDF_CXX_LIBRARY}"
"    Include: ${NetCDF_INCLUDE_DIR}"
"    Include (CXX): ${NetCDF_CXX_INCLUDE_DIR}"
"    Parallel: ${NetCDF_HAS_PARALLEL}")
ngen_dependent_multiline_message(NGEN_WITH_SQLITE
"  SQLite:"
"    Version: ${SQLite3_VERSION}"
"    Library: ${SQLite3_LIBRARIES}"
"    Include: ${SQLite3_INCLUDE_DIRS}")
ngen_dependent_multiline_message(NGEN_WITH_UDUNITS
"  UDUNITS2:"
"    Library: ${UDUNITS2_LIBRARY}"
"    Include: ${UDUNITS2_INCLUDE_DIR}")
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

configure_file("${NGEN_INC_DIR}/NGenConfig.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/NGenConfig.h")
