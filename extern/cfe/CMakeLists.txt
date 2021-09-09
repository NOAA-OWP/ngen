cmake_minimum_required(VERSION 3.12)

# Uncomment this and rebuild artifacts to enable debugging
set(CMAKE_BUILD_TYPE Debug)

project(cfebmi VERSION 1.0.0 DESCRIPTION "OWP CFE BMI Module Shared Library")

# CFE
set(CFE_LIB_NAME_CMAKE cfebmi)
set(CFE_LIB_DESC_CMAKE "OWP CFE BMI Module Shared Library")

# Make sure these are compiled with this directive
add_compile_definitions(BMI_ACTIVE)

if(WIN32)
    add_library(cfebmi cfe/src/bmi_cfe.c cfe/src/cfe.c)
else()
    add_library(cfebmi SHARED cfe/src/bmi_cfe.c cfe/src/cfe.c)
endif()

target_include_directories(cfebmi PRIVATE cfe/include)

set_target_properties(cfebmi PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(cfebmi PROPERTIES PUBLIC_HEADER cfe/bmi_cfe.h)

include(GNUInstallDirs)

install(TARGETS cfebmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(cfebmi.pc.in cfebmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/cfebmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
