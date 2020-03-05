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

set(APP_INCLUDE_DIR ./include)
set(APP_MODELS_DIR ./models)

set(HYMOD_DIR ${APP_MODELS_DIR}/hymod)
set(HYMOD_INCLUDE_DIR ${HYMOD_DIR}/include)

include_directories(${APP_INCLUDE_DIR}/core
        ${APP_INCLUDE_DIR}/core/catchment
        ${APP_INCLUDE_DIR}/core/nexus
        ${APP_INCLUDE_DIR}/core/utility
        ${APP_INCLUDE_DIR}/realizations/catchment
        ${APP_MODELS_DIR}
        ${APP_MODELS_DIR}/kernels
        #${HYMOD_INCLUDE_DIR}
        )

add_executable(ngen
        src/core/NGen.cpp
        src/core/GM_Object.cpp
        src/core/HY_HydroFeature.cpp
        src/core/catchment/HY_CartographicRealization.cpp
        src/core/catchment/HY_Catchment.cpp
        src/core/catchment/HY_CatchmentAggregate.cpp
        src/core/catchment/HY_CatchmentArea.cpp
        src/core/catchment/HY_CatchmentDivide.cpp
        src/core/catchment/HY_CatchmentRealization.cpp
        src/core/catchment/HY_DendriticCatchment.cpp
        src/core/catchment/HY_FlowPath.cpp
        src/core/catchment/HY_HydroNetwork.cpp
        src/core/catchment/HY_InteriorCatchment.cpp
        src/core/nexus/HY_HydroNexus.cpp
        src/core/nexus/HY_PointHydroNexus.cpp
        src/realizations/catchment/Simple_Lumped_Model_Realization.cpp)

# Find the Boost library and configure usage
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

# TODO: add to documentation that a developer must have Boost locally and set the BOOST_ROOT environment var
if(DEFINED ENV{BOOST_ROOT})
    set(Boost_INCLUDE_DIR $ENV{BOOST_ROOT})
else()
    message(STATUS "No Boost root: $ENV{BOOST_ROOT}")
endif()

# TODO: consider setting a minimum or required version
find_package(Boost)

# Link appropriate libraries to main ngen executable target
target_link_libraries(ngen PUBLIC
        Boost::boost                # Headers-only Boost
        )

# Adding to make sure location of generated config header file in binary tree is in include path
target_include_directories(ngen PUBLIC
        "${PROJECT_BINARY_DIR}/include"
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
