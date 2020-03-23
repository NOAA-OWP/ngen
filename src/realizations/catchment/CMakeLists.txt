cmake_minimum_required(VERSION 3.10)

add_library(NGen::realizations_catchment STATIC
        Simple_Lumped_Model_Realization.cpp
        )

target_include_directories(NGen::realizations_catchment PUBLIC
        ../../../include/core
        ../../../include/core/catchment
        ../../../models
        ../../../models/kernels
        )

target_link_libraries(NGen::realizations_catchment PUBLIC
        NGen::core_catchment
        )
# TODO: confirm that don't need to link a library for "LinearReservoir.hpp"
