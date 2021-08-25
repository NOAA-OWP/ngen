cmake_minimum_required(VERSION 3.12)
project(iso_c_fortran_test VERSION 1.0.0 DESCRIPTION "Test executable for driving a fortran BMI model from a C executable")

add_subdirectory(../../noah-mp-modular ${CMAKE_BINARY_DIR}/noahmp)

add_executable( test test_iso_c.c)
target_link_libraries( test surfacebmi iso_c_bmi 
                        ${NETCDF_LIBRARIES}
                        netcdff)

