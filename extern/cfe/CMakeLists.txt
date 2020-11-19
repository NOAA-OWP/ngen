cmake_minimum_required(VERSION 3.10)

project(cfemodel VERSION 1.0.0 DESCRIPTION "External CFE Model Shared Library")

if(WIN32)
    add_library(cfemodel src/bmi_cfe.c src/cfe.c)
else()
    add_library(cfemodel SHARED src/bmi_cfe.c src/cfe.c)
endif()

target_include_directories(cfemodel PRIVATE include)

set_target_properties(cfemodel PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(cfemodel PROPERTIES PUBLIC_HEADER include/bmi_cfe.h)

include(GNUInstallDirs)

install(TARGETS cfemodel
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(cfemodel.pc.in cfemodel.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/cfemodel.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)