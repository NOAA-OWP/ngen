cmake_minimum_required(VERSION 3.12)
enable_language( Fortran )
add_subdirectory(../iso_c_fortran_bmi ${CMAKE_BINARY_DIR}/iso_c_bmi)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../cmake/")
# Uncomment this and rebuild artifacts to enable debugging
#set(CMAKE_BUILD_TYPE Debug)
add_compile_options(
  "-Wall"
  "$<$<CONFIG:DEBUG>:-g;-fbacktrace;-fbounds-check>"
)
project(noah-mp-modular VERSION 1.0.0 DESCRIPTION "External BMI Models Shared Libraries")

set( CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/fortran)

#### Add variables for individual libraries that are used withing the *.pc.in files
# Surface 
set(SURFACE_LIB_NAME_CMAKE surfacebmi)
set(SURFACE_LIB_DESC_CMAKE "External Noah-MP Surface Shared Library")

# Make sure these are compiled with this directive
add_compile_definitions(BMI_ACTIVE)

find_package (netCDF COMPONENTS F90 CONFIG)
if(netCDF_FOUND)
  message("Found netCDF config package")
else()
  # Try the VTK like FindNetCDF.cmake
  find_package(NetCDF COMPONENTS F90 MODULE)
  if(NetCDF_FOUND)
    message("Found netCDF package")
    set(netCDF_INCLUDE_DIR ${NetCDF_INCLUDE_DIRS} CACHE PATH "NetCDF Include Directories" )
    message("NetCDF inc: " ${NetCDF_INCLUDE_DIR})
    set(netCDF_C_LIB ${NetCDF_LIBRARIES} CACHE PATH "NetCDF Libraries to link" )
    message("NetCDF lib: " ${NetCDF_LIBRARIES})
  else()
      message(SEND_ERROR "Cound not find NetCDF package")
  endif()
endif()
# Ensure we can find the netcdf.mod file, leave a cache var that can be set if we can't
find_path(netCDF_MOD_PATH "netcdf.mod" ${netCDF_INCLUDE_DIR})
if(NOT netCDF_MOD_PATH)
  message(SEND_ERROR "no netcdf fortran module found")
endif()

# For some reason the netcdf config.cmake doesn't set the netcdff lib correctly in
# ${netCDF_Libararies}, so it only links the C lib (netcdf) and not the fortran lib (netcdff)
# So find that library explicitly and make sure we link with it.
# also, the FindNetCDF.cmake doesn't seem to do it correctly either.
find_library(netCDF_FORTRAN_LIB netcdff REQUIRED)

set(MODEL_SOURCE_DIR noah-mp-modular/modules/surface_bmi/src/)
set(BMI_SOURCE_DIR noah-mp-modular/modules/surface_bmi/bmi/)

file(GLOB BMI_SOURCE ${BMI_SOURCE_DIR}*.f90)
file( GLOB MODEL_SOURCES ${MODEL_SOURCE_DIR}*.f90)

list(APPEND MODEL_SOURCES noah-mp-modular/modules/surface_bmi/driver/noahmp_ascii_read.f90
                          noah-mp-modular/modules/surface_bmi/driver/noahmp_output.f90)

#add_library(surfacebmi_obj STATIC ${MODEL_SOURCES} )
#target_compile_options(surfacebmi_obj PUBLIC -cpp -ffree-line-length-none)
#target_include_directories(surfacebmi_obj PUBLIC noah-mp-modular/modules/surface_bmi/driver)

#target_include_directories(surfacebmi_obj PUBLIC ${netCDF_INCLUDE_DIR})
#ensure relocatable object, make a different obj lib for a static lib and turn this off
#set_property(TARGET surfacebmi_obj PROPERTY POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    add_library(surfacebmi ${BMI_SOURCE} )
else()
    add_library(surfacebmi SHARED ${BMI_SOURCE} ${MODEL_SOURCES} )
    #add_library(surfacebmi_a STATIC ${BMI_SOURCE})
endif()

target_include_directories(surfacebmi PUBLIC ${netCDF_INCLUDE_DIR} ${netCDF_MOD_PATH})
#We know we are building this for NGEN support, so define the NGEN_ACTIVE preprocessor directive in the compile options
target_compile_options(surfacebmi PUBLIC -DNGEN_ACTIVE -cpp -ffree-line-length-none)

target_link_libraries( surfacebmi iso_c_bmi ${netCDF_C_LIB} ${netCDF_FORTRAN_LIB})
set_target_properties(surfacebmi PROPERTIES VERSION ${PROJECT_VERSION})

#TODO is this needed for fortran?
#set_target_properties(surfacebmi PROPERTIES PUBLIC_HEADER ${BMI_SOURCE})

include(GNUInstallDirs)

install(TARGETS surfacebmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(surfacebmi.pc.in surfacebmi.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/surfacebmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)