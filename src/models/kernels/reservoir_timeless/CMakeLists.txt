cmake_minimum_required(VERSION 3.10)
add_library(kernels_reservoir_timeless STATIC
        Reservoir_Timeless.cpp
        Reservoir_Outlet_Timeless.cpp
        Reservoir_Exponential_Outlet_Timeless.cpp
        Reservoir_Linear_Outlet_Timeless.cpp
        )
add_library(NGen::kernels_reservoir_timeless ALIAS kernels_reservoir_timeless)
target_include_directories(kernels_reservoir_timeless PUBLIC
        ${PROJECT_SOURCE_DIR}/models/kernels/reservoir_timeless
        )
