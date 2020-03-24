cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(realizations_catchment "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::realizations_catchment ALIAS realizations_catchment)

target_include_directories(realizations_catchment PUBLIC
        ../../../include/core
        ../../../include/core/catchment
        ../../../include/realizations/catchment
        ../../../models
        ../../../models/kernels
        )

target_link_libraries(realizations_catchment PUBLIC
        NGen::core_catchment
        )
# TODO: confirm that don't need to link a library for "LinearReservoir.hpp"
