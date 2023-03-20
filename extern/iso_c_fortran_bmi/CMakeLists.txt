cmake_minimum_required(VERSION 3.12)

project(iso_c_fortran_bmi VERSION 1.0.0 DESCRIPTION "Shared library for ISO_C_BINDING Fortran Libraries implementing BMI")

enable_language( Fortran )

# https://www.intel.com/content/www/us/en/develop/documentation/oneapi-dpcpp-cpp-compiler-dev-guide-and-reference/top/compiler-setup/use-the-command-line/use-cmake-with-the-compiler.html
if (INTEL_DPCPP)
    cmake_minimum_required(VERSION 3.20)
    find_package(IntelDPCPP REQUIRED)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake/")
# Uncomment this and rebuild artifacts to enable debugging
#set(CMAKE_BUILD_TYPE Debug)
add_compile_options(
  "-Wall"
  "$<$<CONFIG:DEBUG>:-g;-fbacktrace;-fbounds-check>"
)

set( CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/fortran)
#### Add variables for individual libraries that are used withing the *.pc.in files
set(SOURCE_DIR src/)
file( GLOB SOURCES ${SOURCE_DIR}*.f90)

if(WIN32)
    add_library(iso_c_bmi ${SOURCES} )
else()
    add_library(iso_c_bmi SHARED ${SOURCES} )
    #add_library(iso_c_bmi_a STATIC ${SOURCES} )
endif()

target_compile_options(iso_c_bmi PUBLIC -cpp -ffree-line-length-none)
#ensure relocatable object
set_property(TARGET iso_c_bmi PROPERTY POSITION_INDEPENDENT_CODE ON)

set_target_properties(iso_c_bmi PROPERTIES VERSION ${PROJECT_VERSION})

include(GNUInstallDirs)

install(TARGETS iso_c_bmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(DIRECTORY ${CMAKE_Fortran_MODULE_DIRECTORY} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(iso_c_bmi.pc.in iso_c_bmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/iso_c_bmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
