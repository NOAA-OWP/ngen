add_library(ngen_python InterpreterUtil.cpp)
add_library(NGen::python ALIAS ngen_python)

target_include_directories(ngen_python PUBLIC ${PROJECT_SOURCE_DIR}/include/)

target_link_libraries(ngen_python PUBLIC
                          NGen::config_header
                      )

if (NGEN_WITH_PYTHON)
    target_link_libraries(ngen_python PUBLIC pybind11::embed)
    configure_file(
        ${PROJECT_SOURCE_DIR}/src/utilities/python/NGen_Python_Build_Versions.in
        ${CMAKE_CURRENT_BINARY_DIR}/NGen_Python_Build_Versions.cpp
    )
    target_sources(ngen_python PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/NGen_Python_Build_Versions.cpp)
endif ()
