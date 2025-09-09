cmake_minimum_required(VERSION 3.12)

# Uncomment this and rebuild artifacts to enable debugging
set(CMAKE_BUILD_TYPE Debug)

project(topmodelbmi VERSION 1.0.0 DESCRIPTION "OWP TOPMODEL BMI Module Shared Library")

set(TOPMODEL_LIB_NAME_CMAKE topmodelbmi)
set(TOPMODEL_LIB_DESC_CMAKE "OWP TOPMODEL BMI Module Shared Library")

# Make sure these are compiled with this directive
add_compile_definitions(BMI_ACTIVE)

# -----------------------------------------------------------------------------
# Find the Boost library and configure usage
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.79.0 REQUIRED COMPONENTS serialization)

if(WIN32)
    add_library(topmodelbmi topmodel/src/bmi_topmodel.c topmodel/src/topmodel.c topmodel/src/logger.c topmodel/src/bmi_serialization.cpp)
else()
    add_library(topmodelbmi SHARED topmodel/src/bmi_topmodel.c topmodel/src/topmodel.c topmodel/src/logger.c topmodel/src/bmi_serialization.cpp)
endif()

target_include_directories(topmodelbmi PRIVATE topmodel/include)

set_target_properties(topmodelbmi PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(topmodelbmi PROPERTIES PUBLIC_HEADER topmodel/include/bmi_topmodel.h)

target_link_libraries(topmodelbmi PRIVATE Boost::serialization)

# Code requires minimum of C99 standard to compile
set_target_properties(topmodelbmi PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

install(TARGETS topmodelbmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(topmodelbmi.pc.in topmodelbmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/topmodelbmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
