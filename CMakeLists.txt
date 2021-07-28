cmake_minimum_required(VERSION 3.10)

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

if (NOT DEFINED CMAKE_C_COMPILER)
    message(STATUS "Checking environment variable 'C' for C compiler")
    if (DEFINED ENV{CC})
        set(CMAKE_C_COMPILER $ENV{CC})
    else()
        message(ERROR "'CC' not set - unable to find C++ compiler")
    endif()
else()
    message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
endif()

if (NOT DEFINED CMAKE_CXX_COMPILER)
    message(STATUS "Checking environment variable 'CXX' for C++ compiler")
    if (DEFINED ENV{CXX})
        set(CMAKE_CXX_COMPILER $ENV{CXX})
    else()
        message(ERROR "'CXX' not set - unable to find C++ compiler")
    endif()
else()
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PROJ_ROOT_INCLUDE_DIR ./include)
set(PROJ_ROOT_MODELS_DIR ./models)

set(HYMOD_DIR ${PROJ_ROOT_MODELS_DIR}/hymod)
set(HYMOD_INCLUDE_DIR ${HYMOD_DIR}/include)

add_executable(ngen
        src/NGen.cpp
        )

# Find the Boost library and configure usage
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

if(DEFINED ENV{BOOST_ROOT})
    #set(Boost_INCLUDE_DIR $ENV{BOOST_ROOT})
    set(BOOST_ROOT $ENV{BOOST_ROOT})
else()
    # Look for version-specific Boost directory if available (known from Github Actions VM docs)
    if(DEFINED ENV{BOOST_ROOT_1_72_0})
        set(BOOST_ROOT $ENV{BOOST_ROOT_1_72_0})
        set(ENV{BOOST_ROOT} $ENV{BOOST_ROOT_1_72_0})
    else()
        message(STATUS "No Boost root: $ENV{BOOST_ROOT}")
    endif()
endif()

if(MPI_ACTIVE)
    find_package(MPI)
    add_compile_definitions(NGEN_MPI_ACTIVE)
endif()

if(LSTM_TORCH_LIB_ACTIVE)
   find_package(Torch REQUIRED)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
endif()

find_package(Boost 1.72.0 REQUIRED)

# Handle several steps for BMI C library logic and dependencies, at top level, if functionality is turned on
if(BMI_C_LIB_ACTIVE)
    # Define associated preprocessor directive
    add_compile_definitions(NGEN_BMI_C_LIB_ACTIVE)
endif()

#### Python stuff
# First, define NGEN_ACTIVATE_PYTHON, if not already defined
#   Use NGEN_ACTIVATE_PYTHON environment variable if available, or default to true
if (NOT (DEFINED NGEN_ACTIVATE_PYTHON))
    message(INFO " NGEN_ACTIVATE_PYTHON not defined")
    if (DEFINED ENV{NGEN_ACTIVATE_PYTHON})
        message(INFO " set as $ENV{NGEN_ACTIVATE_PYTHON}")
        set(NGEN_ACTIVATE_PYTHON $ENV{NGEN_ACTIVATE_PYTHON})
    else()
        set(NGEN_ACTIVATE_PYTHON true)
    endif()
endif()

# If Python set to be active, run steps to make sure it is included properly, and set compiler definition to true
if (${NGEN_ACTIVATE_PYTHON})
    add_compile_definitions(ACTIVATE_PYTHON=true)
    # If there is a virtual environment directory at ./.venv, then look for numpy there
    if(EXISTS ${PROJECT_SOURCE_DIR}/.venv)
        file(GLOB_RECURSE NP_COMMON_HEADER ${PROJECT_SOURCE_DIR}/.venv/*npy_common.h)
        get_filename_component(NP_HEADER_DIR ${NP_COMMON_HEADER} DIRECTORY)
        get_filename_component(NP_INCLUDE_DIR ${NP_HEADER_DIR} DIRECTORY)
        set(Python_NumPy_INCLUDE_DIR ${NP_INCLUDE_DIR})
        message(INFO " Found numpy include path  ${Python_NumPy_INCLUDE_DIR}")
        # Alternatively, if there is something in the ENV for the user, try that
    elseif(DEFINED ENV{Python_NumPy_INCLUDE_DIR})
        set(Python_NumPy_INCLUDE_DIR $ENV{Python_NumPy_INCLUDE_DIR})
    endif()

    if(DEFINED ENV{PYTHON_INCLUDE_DIR})
        set(PYTHON_INCLUDE_DIR $ENV{PYTHON_INCLUDE_DIR})
        message(INFO " Found 'PYTHON_INCLUDE_DIR' from ENV ($ENV{PYTHON_INCLUDE_DIR})")
    else()
        message(INFO " Did not find 'PYTHON_INCLUDE_DIR' in ENV")
    endif()
    if(DEFINED ENV{PYTHON_LIBRARIES})
        set(PYTHON_LIBRARIES $ENV{PYTHON_LIBRARIES})
        message(INFO " Found 'PYTHON_LIBRARIES' from ENV ($ENV{PYTHON_LIBRARIES})")
    else()
        message(INFO " Did not find 'PYTHON_LIBRARIES' in ENV")
    endif()
    include_directories(${PYTHON_INCLUDE_DIR})

    find_package(Python 3.6.8 REQUIRED COMPONENTS Development NumPy)
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

add_subdirectory("src/core")
add_subdirectory("src/geojson")
add_subdirectory("src/realizations/catchment")
add_subdirectory("src/models/tshirt")
add_subdirectory("src/models/kernels/reservoir")
add_subdirectory("src/models/kernels/evapotranspiration")
#add_subdirectory("src/forcing")

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
        #NGen::forcing
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

#add_library(Hymod ${HYMOD_INCLUDE_DIR}/Hymod.h)
#set_target_properties(Hymod PROPERTIES LINKER_LANGUAGE CXX)

#target_compile_features(Hymod PUBLIC cxx_std_14)

#set_target_properties(ngen PROPERTIES LINKER_LANGUAGE CXX)
#
#target_compile_options(ngen PUBLIC -std=c++14 -Wall)
#target_compile_features(ngen PUBLIC cxx_std_14)
#add_subdirectory("src/geojson")
