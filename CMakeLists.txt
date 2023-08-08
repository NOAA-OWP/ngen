cmake_minimum_required(VERSION 3.14)

if(MPI_ACTIVE)
   set (CMAKE_CXX_COMPILER "mpicxx")
endif()

########### Define project version and use via generated config header
project(ngen VERSION 0.1.0)
configure_file(include/NGenConfig.h.in include/NGenConfig.h)

if("${PROJECT_NAME}" STREQUAL "${CMAKE_PROJECT_NAME}")
    option(PACKAGE_TESTS "Build automated tests" ON)
else()
    option(PACKAGE_TESTS "Build automated tests")
endif()

# https://www.intel.com/content/www/us/en/develop/documentation/oneapi-dpcpp-cpp-compiler-dev-guide-and-reference/top/compiler-setup/use-the-command-line/use-cmake-with-the-compiler.html
if (INTEL_DPCPP)
    find_package(IntelDPCPP REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

include(ExternalProject)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# Account for different OS and how that impacts shared lib file names
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    set(SHARED_LIB_EXTENSION "dylib")
elseif(WIN32)
    error( "Windows platforms are not currently supported" )
else()
    set(SHARED_LIB_EXTENSION "so")
endif()

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
endif()
function(git_update_submodule PATH)
    if(GIT_SUBMODULE)
        message(STATUS "Updating submodule ${PATH}")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init -- ${PATH}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT
            )
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init for \"${PATH}\" failed with ${GIT_SUBMOD_RESULT}")
        endif()
    else()
        message(FATAL_ERROR "Git not detected: cannot obtain submodule at \"${PATH}\" (but how is this even working?)")
    endif()
endfunction()

set(PROJ_ROOT_INCLUDE_DIR ./include)
set(PROJ_ROOT_MODELS_DIR ./models)

# Find the Boost library and configure usage
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

if(NOT DEFINED BOOST_ROOT AND NOT DEFINED ENV{BOOST_ROOT})
    # Look for version-specific Boost directory if available (known from Github Actions VM docs)
    if(DEFINED ENV{BOOST_ROOT_1_72_0})
        set(BOOST_ROOT $ENV{BOOST_ROOT_1_72_0})
        message(STATUS "Using value of \$ENV{BOOST_ROOT_1_72_0} to set BOOST_ROOT=$ENV{BOOST_ROOT_1_72_0}")
    else()
        message(STATUS "No BOOST_ROOT set")
    endif()
endif()

find_package(Boost 1.72.0 REQUIRED)

if(MPI_ACTIVE)
    find_package(MPI)
    add_compile_definitions(NGEN_MPI_ACTIVE)
endif()

if(LSTM_TORCH_LIB_ACTIVE)
   find_package(Torch REQUIRED)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
endif()

find_package(SQLite3)
if(SQLite3_FOUND)
    set(NGEN_WITH_SQLITE3 ON)
    add_compile_definitions(NGEN_WITH_SQLITE3)
else()
    set(NGEN_WITH_SQLITE3 OFF)
endif()
message(INFO " SQLite3 support is ${NGEN_WITH_SQLITE3}")

# UDUNITS
# Since UDUNITS is currently not really optional (yet) let's make it default to on...
if (NOT (DEFINED UDUNITS_ACTIVE))
    message(INFO " UDUNITS_ACTIVE not defined")
    if (DEFINED ENV{UDUNITS_ACTIVE})
        message(INFO " set as $ENV{UDUNITS_ACTIVE}")
        set(UDUNITS_ACTIVE $ENV{UDUNITS_ACTIVE})
    else()
        set(UDUNITS_ACTIVE true)
    endif()
endif()

if(UDUNITS_ACTIVE)
    find_package(UDUNITS2 REQUIRED)
    if(UDUNITS2_LIBRARY)
        #add_library(libudunits2 SHARED IMPORTED)
        target_include_directories(libudunits2 INTERFACE ${UDUNITS2_INCLUDE})
        #include_directories(${UDUNITS2_INCLUDE})
        message("INFO Using UDUNITS2 at ${UDUNITS2_LIBRARY} and ${UDUNITS2_INCLUDE}")
    endif()
    #add_compile_definitions("NGEN_UDUNITS2_XML_PATH=\"${UDUNITS_INSTALL_DIR}/share/udunits/udunits2.xml\"")
endif()

cmake_policy(SET CMP0074 NEW)
if(NETCDF_ACTIVE)
    set (NETCDF_CXX "YES")
    set (NETCDF_F77 "NO")
    set (NETCDF_F90 "NO")
    find_package(NetCDF REQUIRED)
    add_compile_definitions(NETCDF_ACTIVE)
    if(NETCDF_FOUND)
        include_directories(${NETCDF_INCLUDE_DIRS})
        if(NETCDF_HAS_PARALLEL)
            message("INFO Using NETCDF (with parallel!) at ${NETCDF_LIBRARIES} and ${NETCDF_INCLUDE_DIRS}")
        else()
            message("INFO Using NETCDF (without parallel!) at ${NETCDF_LIBRARIES} and ${NETCDF_INCLUDE_DIRS}")
        endif()
    else()
        message("INFO netCDF library not found")
    endif()
endif()

add_executable(ngen
    src/NGen.cpp
    )

# Natively support BMI C++ modules and pre-compile in the test_bmi_cpp mock/example.
set(TEST_BMI_CPP_DIR ${PROJECT_SOURCE_DIR}/extern/test_bmi_cpp)
git_update_submodule(${PROJECT_SOURCE_DIR}/extern/bmi-cxx)
add_subdirectory(${TEST_BMI_CPP_DIR} ${TEST_BMI_CPP_DIR}/cmake_build)
add_dependencies(ngen testbmicppmodel) # because our demo realizations will use it

# Handle several steps for BMI C library logic and dependencies, at top level, if functionality is turned on
if(BMI_C_LIB_ACTIVE)
    # Define associated preprocessor directive
    add_compile_definitions(NGEN_BMI_C_LIB_ACTIVE)
endif()

# Configure whether Fortran BMI functionality is active
if (${BMI_FORTRAN_ACTIVE})
    add_compile_definitions(NGEN_BMI_FORTRAN_ACTIVE)
endif()

#### Python stuff
# First, define NGEN_ACTIVATE_PYTHON, if not already defined
#   Use NGEN_ACTIVATE_PYTHON environment variable if available, or default to true
if (NOT (DEFINED NGEN_ACTIVATE_PYTHON))
    if (DEFINED ENV{NGEN_ACTIVATE_PYTHON})
        message(INFO " NGEN_ACTIVATE_PYTHON set to '$ENV{NGEN_ACTIVATE_PYTHON}' from the environment")
        set(NGEN_ACTIVATE_PYTHON $ENV{NGEN_ACTIVATE_PYTHON})
    else()
        message(INFO " NGEN_ACTIVATE_PYTHON set to 'true' by default")
        set(NGEN_ACTIVATE_PYTHON true)
    endif()
endif()

# If Python set to be active, run steps to make sure it is included properly, and set compiler definition to true
if (${NGEN_ACTIVATE_PYTHON})
    add_compile_definitions(ACTIVATE_PYTHON=true)

    find_package(Python 3.6.8 REQUIRED COMPONENTS Interpreter Development NumPy)
    #message(INFO " the final Py lib is ${Python_LIBRARY}")
    #message(INFO " the final Py libraries are ${Python_LIBRARIES}")
    #message(INFO " the final Py library dirs are ${Python_LIBRARY_DIRS}")
    #message(INFO " the final Py runtime library dirs are ${Python_RUNTIME_LIBRARY_DIRS}")
    #message(INFO " the final Py std lib is ${Python_STDLIB}")
    #message(INFO " the rpath is ${CMAKE_INSTALL_RPATH}")
    add_subdirectory(extern/pybind11)
endif()

target_include_directories(ngen PUBLIC
        "${PROJECT_BINARY_DIR}/include" # For generated config header file in binary tree
        )

  
# In some IDEs (e.g., CLion), it is easier to work with the code in the included example of an "external" shared library
# when that code appears to be part of the main project, which can be done by add a statement like the ones below.
#
# However, these should be removed or commented out when running CMake builds and in the committed version of this file.
#
#add_subdirectory("extern/cfe")
#add_subdirectory("extern/test_bmi_c")

if(QUIET)
set(UDUNITS_QUIET true)
set(ET_QUIET true)
set(NGEN_QUIET true)
endif()

if(UDUNITS_QUIET)
add_compile_definitions(UDUNITS_QUIET)
endif()

if(ET_QUIET)
add_compile_definitions(ET_QUIET)
endif()

if(NGEN_QUIET)
add_compile_definitions(NGEN_QUIET)
endif()

add_subdirectory("src/core")
add_dependencies(core libudunits2)
add_subdirectory("src/geojson")

if(NGEN_WITH_SQLITE3)
    add_subdirectory("src/geopackage")
    target_link_libraries(ngen PUBLIC NGen::geopackage)
endif()

add_subdirectory("src/realizations/catchment")
add_subdirectory("src/models/tshirt")
add_subdirectory("src/models/kernels/reservoir")
add_subdirectory("src/models/kernels/evapotranspiration")
add_subdirectory("src/forcing")

target_link_libraries(ngen PUBLIC
        NGen::core
        NGen::core_catchment
        #NGen::core_catchment_giuh
        NGen::core_nexus
        NGen::geojson
        NGen::models_tshirt
        NGen::realizations_catchment
        NGen::kernels_reservoir
        NGen::kernels_evapotranspiration
        #NGen::kernels_reservoir_timeless
        NGen::forcing
        NGen::core_mediator
        libudunits2
        ${NETCDF_LIBRARIES}
        )

add_executable(partitionGenerator
    src/partitionGenerator.cpp
    )

target_include_directories(partitionGenerator PUBLIC
        "${PROJECT_BINARY_DIR}/include" # For generated config header file in binary tree
        )

    target_link_libraries(partitionGenerator PUBLIC
       core
       geojson
       )

if(NGEN_ACTIVATE_ROUTING)
    add_compile_definitions(NGEN_ROUTING_ACTIVE)
    add_subdirectory("src/routing")
    target_link_libraries(ngen PUBLIC
            NGen::routing
           )
endif()

if(LSTM_TORCH_LIB_ACTIVE)
    add_compile_definitions(NGEN_LSTM_TORCH_LIB_ACTIVE)
    add_subdirectory("src/models/lstm")
    target_link_libraries(ngen PUBLIC
            NGen::models_lstm
            ${TORCH_LIBRARIES}
           )
endif()

# For automated testing with Google Test
if(PACKAGE_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(test)
endif()

#set_target_properties(ngen PROPERTIES LINKER_LANGUAGE CXX)
#
#target_compile_options(ngen PUBLIC -std=c++14 -Wall)
#target_compile_features(ngen PUBLIC cxx_std_14)
#add_subdirectory("src/geojson")
