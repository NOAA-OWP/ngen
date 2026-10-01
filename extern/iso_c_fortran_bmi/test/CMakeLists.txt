cmake_minimum_required(VERSION 3.12)
project(iso_c_fortran_test VERSION 1.0.0 DESCRIPTION "Test executable for driving a fortran BMI model from a C executable")

#test_bmi_fortran provides the required iso_c_bmi
add_subdirectory(../../test_bmi_fortran ${CMAKE_BINARY_DIR}/test_bmi_fortran)

find_package(Threads REQUIRED)

add_executable( test test_iso_c.c)
target_link_libraries( test testbmifortranmodel iso_c_bmi )

# Concurrency regression test for the shim's string conversion.
# Built, but deliberately not registered with CTest: it is a stress test meant
# to be run by hand, not on every `ctest` invocation.
add_executable( test_threads test_iso_c_threads.c)
target_link_libraries( test_threads testbmifortranmodel iso_c_bmi Threads::Threads )
