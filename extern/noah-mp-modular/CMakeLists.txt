cmake_minimum_required(VERSION 3.12)
enable_language( Fortran )
add_subdirectory(../iso_c_fortran_bmi ${CMAKE_BINARY_DIR}/iso_c_bmi)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake/")
# Uncomment this and rebuild artifacts to enable debugging
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -g -fbacktrace -fbounds-check -Wall")
project(noah-mp-modular VERSION 1.0.0 DESCRIPTION "External BMI Models Shared Libraries")

set( CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/fortran)

#### Add variables for individual libraries that are used withing the *.pc.in files
# Surface 
set(SURFACE_LIB_NAME_CMAKE surfacebmi)
set(SURFACE_LIB_DESC_CMAKE "External Noah-MP Surface Shared Library")

# Make sure these are compiled with this directive
add_compile_definitions(BMI_ACTIVE)
find_package (NetCDF REQUIRED COMPONENTS F90)

set(MODEL_SOURCE_DIR noah-mp-modular/modules/surface_bmi/src/)
set(BMI_SOURCE_DIR noah-mp-modular/modules/surface_bmi/bmi/)

file(GLOB BMI_SOURCE ${BMI_SOURCE_DIR}*.f90)
file( GLOB MODEL_SOURCES ${MODEL_SOURCE_DIR}*.f90)

list(APPEND MODEL_SOURCES noah-mp-modular/modules/surface_bmi/driver/noahmp_ascii_read.f90
                          noah-mp-modular/modules/surface_bmi/driver/noahmp_output.f90)

add_library(surfacebmi_obj OBJECT ${MODEL_SOURCES} )
target_compile_options(surfacebmi_obj PUBLIC -cpp -ffree-line-length-none)
target_include_directories(surfacebmi_obj PUBLIC noah-mp-modular/modules/surface_bmi/driver)

#HACK FIXME RM
set(NETCDF_F90_INCLUDE_DIRS /usr/include/openmpi-x86_64/)
target_include_directories(surfacebmi_obj PUBLIC ${NETCDF_F90_INCLUDE_DIRS})
#ensure relocatable object, make a different obj lib for a static lib and turn this off?
set_property(TARGET surfacebmi_obj PROPERTY POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    add_library(surfacebmi ${BMI_SOURCE} )
else()
    add_library(surfacebmi SHARED ${BMI_SOURCE} )
    #add_library(surfacebmi_a STATIC ${BMI_SOURCE})
endif()

#TODO is this needed for fortran?
target_include_directories(surfacebmi PRIVATE ${MODEL_SOURCE_DIR})
target_link_libraries( surfacebmi surfacebmi_obj )
#target_link_libraries( surfacebmi_a surfacebmi_obj )
target_link_libraries( surfacebmi iso_c_bmi )
#target_link_libraries( surfacebmi_a iso_c_bmi_a )
set_target_properties(surfacebmi PROPERTIES VERSION ${PROJECT_VERSION})

#TODO is this needed for fortran?
#set_target_properties(surfacebmi PROPERTIES PUBLIC_HEADER ${BMI_SOURCE})

include(GNUInstallDirs)


install(TARGETS surfacebmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(surfacebmi.pc.in surfacebmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/surfacebmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)