cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(realizations_catchment "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::realizations_catchment ALIAS realizations_catchment)

target_include_directories(realizations_catchment PUBLIC
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/catchment
        ${PROJECT_SOURCE_DIR}/include/realizations/catchment
        ${PROJECT_SOURCE_DIR}/include/forcing
        ${PROJECT_SOURCE_DIR}/include/simulation_time
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/include/geojson
        ${PROJECT_SOURCE_DIR}/models
        ${PROJECT_SOURCE_DIR}/models/kernels
        ${PROJECT_SOURCE_DIR}/models/kernels/evapotranspiration
        ${PROJECT_SOURCE_DIR}/models/tshirt/include
        )
        
target_link_libraries(realizations_catchment PUBLIC
        ${CMAKE_DL_LIBS}
        NGen::core_catchment
        NGen::core_catchment_giuh
        NGen::models_tshirt
        NGen::geojson
        NGen::kernels_evapotranspiration
        )

if(NGEN_ACTIVATE_PYTHON)
   target_include_directories(realizations_catchment PUBLIC ${PROJECT_SOURCE_DIR}/extern/pybind11/include)
   target_link_libraries(realizations_catchment PUBLIC pybind11::embed)
endif()

add_dependencies(realizations_catchment NGen::kernels_evapotranspiration)

if(LSTM_TORCH_LIB_ACTIVE)
   add_compile_definitions(NGEN_LSTM_TORCH_LIB_ACTIVE)

   target_include_directories(realizations_catchment PUBLIC
        ${PROJECT_SOURCE_DIR}/models/lstm/include
        )
   target_link_libraries(realizations_catchment PUBLIC
        NGen::models_lstm
        )
endif()
