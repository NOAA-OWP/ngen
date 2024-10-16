include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(realizations_coastal "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::realizations_coastal ALIAS realizations_coastal)

target_include_directories(realizations_coastal PUBLIC
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/realizations/coastal
        ${PROJECT_SOURCE_DIR}/include/forcing
        ${PROJECT_SOURCE_DIR}/include/simulation_time
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/include/bmi
        )
        
target_link_libraries(realizations_coastal PUBLIC
        ${CMAKE_DL_LIBS}
        NGen::config_header
        NGen::geojson
        NGen::logging
        NGen::ngen_bmi
        )

