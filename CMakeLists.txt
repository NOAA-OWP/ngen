cmake_minimum_required(VERSION 3.10)
########### Define project version and use via generated config header
project(ngen VERSION 0.1.0)
configure_file(include/NGenConfig.h.in include/NGenConfig.h)

if("${PROJECT_NAME}" STREQUAL "${CMAKE_PROJECT_NAME}")
    option(PACKAGE_TESTS "Build automated tests" ON)
else()
    option(PACKAGE_TESTS "Build automated tests")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(APP_INCLUDE_DIR ./include)
set(APP_MODELS_DIR ./models)

set(HYMOD_DIR ${APP_MODELS_DIR}/hymod)
set(HYMOD_INCLUDE_DIR ${HYMOD_DIR}/include)

include_directories(${APP_INCLUDE_DIR})
include_directories(${HYMOD_INCLUDE_DIR})

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
