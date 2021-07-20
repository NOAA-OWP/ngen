cmake_minimum_required(VERSION 3.10)

# Uncomment this and rebuild artifacts to enable debugging
set(CMAKE_BUILD_TYPE Debug)

project(testbmicmodel VERSION 1.0.0 DESCRIPTION "BMI C Testing Model Shared Library")

if(WIN32)
    add_library(testbmicmodel src/bmi_test_bmi_c.c src/test_bmi_c.c)
else()
    add_library(testbmicmodel SHARED src/bmi_test_bmi_c.c src/test_bmi_c.c)
endif()

target_include_directories(testbmicmodel PRIVATE include)

set_target_properties(testbmicmodel PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(testbmicmodel PROPERTIES PUBLIC_HEADER include/bmi_test_bmi_c.h)

include(GNUInstallDirs)

install(TARGETS testbmicmodel
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(testbmicmodel.pc.in testbmicmodel.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/testbmicmodel.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)