cmake_minimum_required(VERSION 3.10)

project(cfe-lib VERSION 1.0.0 DESCRIPTION "External CFE Shared Library")

if(WIN32)
    add_library(cfe-lib src/bmi_cfe.c src/cfe.c)
else()
    add_library(cfe-lib SHARED src/bmi_cfe.c src/cfe.c)
endif()

target_include_directories(cfe-lib PRIVATE include)

set_target_properties(cfe-lib PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(cfe-lib PROPERTIES PUBLIC_HEADER include/bmi_cfe.h)

include(GNUInstallDirs)

install(TARGETS cfe-lib
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(cfe-lib.pc.in cfe-lib.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/cfe-lib.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)