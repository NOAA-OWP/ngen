cmake_minimum_required(VERSION 3.10)

# Uncomment this and rebuild artifacts to enable debugging
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(testbmicppmodel VERSION 1.0.0 DESCRIPTION "BMI C++ Testing Model Shared Library")

if(WIN32)
    add_library(testbmicppmodel src/test_bmi_cpp.cpp)
else()
    add_library(testbmicppmodel SHARED src/test_bmi_cpp.cpp)
endif()

target_include_directories(testbmicppmodel PRIVATE include)

set_target_properties(testbmicppmodel PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(testbmicppmodel PROPERTIES PUBLIC_HEADER include/test_bmi_cpp.hpp)

include(GNUInstallDirs)

install(TARGETS testbmicppmodel
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(testbmicppmodel.pc.in testbmicppmodel.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/testbmicppmodel.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
