cmake_minimum_required(VERSION 3.12...3.26)

project(testbmifortranmodel
    VERSION 1.0.0
    DESCRIPTION "BMI Fortran Testing Model Shared Library"
    LANGUAGES Fortran)

add_library(testbmifortranmodel SHARED)
file(GLOB _TESTBMIFORTRAN_OBJECTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.f90")
get_target_property(_TESTBMIFORTRAN_BINARY_DIR testbmifortranmodel BINARY_DIR)
set_target_properties(testbmifortranmodel
    PROPERTIES
        Fortran_MODULE_DIRECTORY "${_TESTBMIFORTRAN_BINARY_DIR}/mod")

target_sources(testbmifortranmodel PRIVATE ${_TESTBMIFORTRAN_OBJECTS})
target_compile_options(testbmifortranmodel PRIVATE -cpp -ffree-line-length-none)
target_compile_definitions(testbmifortranmodel PRIVATE NGEN_ACTIVE)
target_include_directories(testbmifortranmodel INTERFACE "${_TESTBMIFORTRAN_BINARY_DIR}/mod")
target_link_libraries(testbmifortranmodel PUBLIC iso_c_bmi)

unset(_TESTBMIFORTRAN_BINARY_DIR)
unset(_TESTBMIFORTRAN_OBJECTS)

include(GNUInstallDirs)
install(TARGETS testbmifortranmodel
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
configure_file(testbmifortranmodel.pc.in testbmifortranmodel.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/testbmifortranmodel.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
