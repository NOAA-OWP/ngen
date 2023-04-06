cmake_minimum_required(VERSION 3.10)

project(testbmifortranmodel VERSION 1.0.0 DESCRIPTION "BMI Fortran Testing Model Shared Library")

enable_language( Fortran )

# https://www.intel.com/content/www/us/en/develop/documentation/oneapi-dpcpp-cpp-compiler-dev-guide-and-reference/top/compiler-setup/use-the-command-line/use-cmake-with-the-compiler.html
if (INTEL_DPCPP)
    cmake_minimum_required(VERSION 3.20)
    find_package(IntelDPCPP REQUIRED)
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -g -traceback -check all -debug all -mcmodel=medium -shared-intel")
else()
    set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -g -fbacktrace -fbounds-check -Wall")
endif()


add_subdirectory(../iso_c_fortran_bmi ${CMAKE_BINARY_DIR}/iso_c_bmi)
#Get the iso_c_fortran binding module to build as part of this build
#add_subdirectory(../iso_c_fortran_bmi ${CMAKE_BINARY_DIR}/iso_c_bmi)

# Uncomment this and rebuild artifacts to enable debugging
set(CMAKE_BUILD_TYPE Debug)

set( CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/fortran)

file(GLOB SOURCES src/*.f90) #TODO why does this not pick up new modules/src files after initial configuration?

if(WIN32)
    add_library(testbmifortranmodel ${SOURCES})
else()
    add_library(testbmifortranmodel SHARED ${SOURCES} )
endif()

target_link_libraries(testbmifortranmodel PUBLIC iso_c_bmi)
target_compile_options(testbmifortranmodel PUBLIC -cpp -DNGEN_ACTIVE)
#target_include_directories(testbmifortranmodel PRIVATE include)

set_target_properties(testbmifortranmodel PROPERTIES VERSION ${PROJECT_VERSION})

#set_target_properties(testbmifortranmodel PROPERTIES PUBLIC_HEADER include/bmi_test_bmi_c.h)

include(GNUInstallDirs)

install(TARGETS testbmifortranmodel
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(testbmifortranmodel.pc.in testbmifortranmodel.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/testbmifortranmodel.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
