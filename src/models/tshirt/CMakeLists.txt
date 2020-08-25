cmake_minimum_required(VERSION 3.10)
add_library(models_tshirt STATIC
        Tshirt.cpp
        tshirt_c.cpp)
add_library(NGen::models_tshirt ALIAS models_tshirt)
target_include_directories(models_tshirt PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/core/catchment/giuh
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/models/kernels
        ${PROJECT_SOURCE_DIR}/models/tshirt/include
        )

target_link_libraries(models_tshirt PUBLIC 
        NGen::kernels_reservoir)
