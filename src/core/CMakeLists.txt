cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core ALIAS core)

target_include_directories(core PUBLIC
        ../../include/core
        ../../include/core/catchment
        )

add_subdirectory("catchment")
add_subdirectory("nexus")