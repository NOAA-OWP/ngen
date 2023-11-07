include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(ngen_bmi "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::ngen_bmi ALIAS ngen_bmi)

target_include_directories(ngen_bmi PUBLIC
        ${PROJECT_SOURCE_DIR}/include/
        ${PROJECT_SOURCE_DIR}/include/bmi
        ${PROJECT_SOURCE_DIR}/include/geojson
        ${PROJECT_SOURCE_DIR}/include/core/mediator
        )
        
target_link_libraries(ngen_bmi PUBLIC
        ${CMAKE_DL_LIBS}
        NGen::logging
        )

if(NGEN_WITH_PYTHON)
   target_link_libraries(ngen_bmi PUBLIC pybind11::embed)
endif()

if(NGEN_WITH_BMI_FORTRAN)
    target_link_libraries(ngen_bmi PUBLIC bmi_fortran_iso_c)
endif()
