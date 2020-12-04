cmake_minimum_required(VERSION 3.10)
add_library(models_lstm STATIC
        LSTM.cpp)
add_library(NGen::models_lstm ALIAS models_lstm)
target_include_directories(models_lstm PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/core/catchment/giuh
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/models/kernels
        ${PROJECT_SOURCE_DIR}/models/lstm/include
        ${TORCH_INCLUDE_DIRS}
        )

#find_package(Torch REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")


#target_link_libraries(models_lstm PUBLIC 
#        NGen::kernels_reservoir
#        NGen::geojson
#        ${TORCH_LIBRARIES})


target_link_libraries(models_lstm PUBLIC 
        NGen::kernels_reservoir)
