cmake_minimum_required(VERSION 3.12)

# Uncomment this and rebuild artifacts to enable debugging
set(CMAKE_BUILD_TYPE Debug)

project(lasambmi VERSION 1.0.0 DESCRIPTION "OWP LASAM BMI Module Shared Library")

# LASAM
set(LASAM_LIB_NAME_CMAKE lasambmi)
set(LASAM_LIB_DESC_CMAKE "OWP LASAM BMI Module Shared Library")

# Make sure these are compiled with this directive
add_compile_definitions(BMI_ACTIVE)

# Based on other submodules pattern, support relative paths from ngen
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LGAR-C")
    set(LGAR_SRC_PREFIX "LGAR-C/")
else()
    set(LGAR_SRC_PREFIX "")
endif()

if(WIN32)
  add_library(lasambmi SHARED
    ${LGAR_SRC_PREFIX}src/bmi_lgar.cxx
    ${LGAR_SRC_PREFIX}src/lgar.cxx
    ${LGAR_SRC_PREFIX}src/soil_funcs.cxx
    ${LGAR_SRC_PREFIX}src/linked_list.cxx
    ${LGAR_SRC_PREFIX}src/mem_funcs.cxx
    ${LGAR_SRC_PREFIX}src/util_funcs.cxx
    ${LGAR_SRC_PREFIX}src/aet.cxx
    ${LGAR_SRC_PREFIX}giuh/giuh.c
    ${LGAR_SRC_PREFIX}include/all.hxx
    ${LGAR_SRC_PREFIX}giuh/giuh.h)
else()
  add_library(lasambmi SHARED
    ${LGAR_SRC_PREFIX}src/bmi_lgar.cxx
    ${LGAR_SRC_PREFIX}src/lgar.cxx
    ${LGAR_SRC_PREFIX}src/soil_funcs.cxx
    ${LGAR_SRC_PREFIX}src/linked_list.cxx
    ${LGAR_SRC_PREFIX}src/mem_funcs.cxx
    ${LGAR_SRC_PREFIX}src/util_funcs.cxx
    ${LGAR_SRC_PREFIX}src/aet.cxx
    ${LGAR_SRC_PREFIX}giuh/giuh.c
    ${LGAR_SRC_PREFIX}include/all.hxx
    ${LGAR_SRC_PREFIX}giuh/giuh.h)
endif()

target_include_directories(lasambmi PRIVATE ${LGAR_SRC_PREFIX}include)

set_target_properties(lasambmi PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(lasambmi PROPERTIES PUBLIC_HEADER ${LGAR_SRC_PREFIX}include/bmi_lgar.hxx)

# Set C++14 standard required for compilation
set_target_properties(lasambmi PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

install(TARGETS lasambmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(${LGAR_SRC_PREFIX}lasambmi.pc.in lasambmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/lasambmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
