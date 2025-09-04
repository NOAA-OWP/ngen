include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(routing "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::routing ALIAS routing)

target_include_directories(routing PUBLIC
        ${PROJECT_SOURCE_DIR}/include/routing
        ${PROJECT_SOURCE_DIR}/extern/pybind11/include
        ${PROJECT_SOURCE_DIR}/include/utilities
        )
        
target_link_libraries(routing PUBLIC
                              NGen::python
                              NGen::config_header
                              )


#target_link_libraries(realizations_catchment PUBLIC
#        ${CMAKE_DL_LIBS}
#        NGen::core_catchment
#        NGen::geojson
#        )
