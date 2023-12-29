cmake_minimum_required(VERSION 3.12...3.26)
include(NGenModules)

project(testbmifortranmodel
    VERSION 1.0.0
    DESCRIPTION "BMI Fortran Testing Model Shared Library"
    LANGUAGES Fortran)

add_fortran_library(testbmifortranmodel SHARED)
file(GLOB _TESTBMIFORTRAN_OBJECTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.f90")
target_sources(testbmifortranmodel PRIVATE ${_TESTBMIFORTRAN_OBJECTS})
target_compile_options(testbmifortranmodel PUBLIC -cpp)
target_compile_definitions(testbmifortranmodel PUBLIC NGEN_ACTIVE)
target_link_libraries(testbmifortranmodel PUBLIC iso_c_bmi)

include(GNUInstallDirs)
install(TARGETS testbmifortranmodel
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
configure_file(testbmifortranmodel.pc.in testbmifortranmodel.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/testbmifortranmodel.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
