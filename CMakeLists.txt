cmake_minimum_required(VERSION 3.10)
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

find_package(Boost 1.69.0 REQUIRED)

target_include_directories(ngen PUBLIC
        "${PROJECT_BINARY_DIR}/include" # For generated config header file in binary tree
        )

add_subdirectory("src/core")
add_subdirectory("src/geojson")
add_subdirectory("src/realizations/catchment")

add_subdirectory("src/models/tshirt")
add_subdirectory("src/models/kernels/reservoir")
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
        #NGen::forcing
        )

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
