cmake_minimum_required(VERSION 3.12)

# Uncomment this and rebuild artifacts to enable debugging
set(CMAKE_BUILD_TYPE Debug)

project(topmodelbmi VERSION 1.0.0 DESCRIPTION "OWP TOPMODEL BMI Module Shared Library")

set(TOPMODEL_LIB_NAME_CMAKE topmodelbmi)
set(TOPMODEL_LIB_DESC_CMAKE "OWP TOPMODEL BMI Module Shared Library")

# Make sure these are compiled with this directive
add_compile_definitions(BMI_ACTIVE)

if(WIN32)
    add_library(topmodelbmi topmodel/src/bmi_topmodel.c topmodel/src/topmodel.c)
else()
    add_library(topmodelbmi SHARED topmodel/src/bmi_topmodel.c topmodel/src/topmodel.c)
endif()

target_include_directories(topmodelbmi PRIVATE topmodel/include)

set_target_properties(topmodelbmi PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(topmodelbmi PROPERTIES PUBLIC_HEADER topmodel/include/bmi_topmodel.h)

include(GNUInstallDirs)

install(TARGETS topmodelbmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(topmodelbmi.pc.in topmodelbmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/topmodelbmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)