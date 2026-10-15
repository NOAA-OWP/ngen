# SUMMA for the NextGen framework.
#
# SUMMA's own build/CMakeLists.txt already carries the USE_NEXTGEN branch, which
# builds the BMI shared library instead of the standalone executables and expects
# the SUMMA source one directory down -- exactly the layout here. So include it
# rather than keeping a copy.

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

include(${CMAKE_CURRENT_SOURCE_DIR}/summa/build/CMakeLists.txt)
