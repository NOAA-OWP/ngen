cmake_minimum_required(VERSION 3.10)
add_library(kernels_reservoir STATIC
        LinearReservoir.cpp
        Nonlinear_Reservoir.cpp)
add_library(NGen::kernels_reservoir ALIAS kernels_reservoir)
target_include_directories(kernels_reservoir PUBLIC
        ${PROJECT_SOURCE_DIR}/models/kernels/reservoir
        )