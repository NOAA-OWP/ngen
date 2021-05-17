cmake_minimum_required(VERSION 3.12)

# Uncomment this and rebuild artifacts to enable debugging
set(CMAKE_BUILD_TYPE Debug)

project(../alt-modular VERSION 1.0.0 DESCRIPTION "External BMI Models Shared Libraries")

#### Add variables for individual libraries that are used withing the *.pc.in files
# CFE
set(CFE_LIB_NAME_CMAKE cfebmi)
set(CFE_LIB_DESC_CMAKE "External CFE Shared Library")
# TOPMODEL
set(TOPMODEL_LIB_NAME_CMAKE topmodelbmi)
set(TOPMODEL_LIB_DESC_CMAKE "External TOPMODEL Shared Library")

if(WIN32)
    #add_library(cfemodel src/bmi_cfe.c src/cfe.c)
    add_library(cfebmi ../alt-modular/alt-modular/Modules/cfe-bmi/src/bmi_cfe.c ../alt-modular/alt-modular/Modules/cfe-bmi/src/cfe.c)
    add_library(topmodelbmi ../alt-modular/alt-modular/Modules/TOPMODEL/src/bmi_topmodel.c ../alt-modular/alt-modular/Modules/TOPMODEL/src/topmodel.c)
else()
    #add_library(cfemodel SHARED src/bmi_cfe.c src/cfe.c)
    add_library(cfebmi SHARED ../alt-modular/alt-modular/Modules/cfe-bmi/src/bmi_cfe.c ../alt-modular/alt-modular/Modules/cfe-bmi/src/cfe.c)
    add_library(topmodelbmi SHARED ../alt-modular/alt-modular/Modules/TOPMODEL/src/bmi_topmodel.c ../alt-modular/alt-modular/Modules/TOPMODEL/src/topmodel.c)
endif()

#target_include_directories(cfemodel PRIVATE include)
target_include_directories(cfebmi PRIVATE ../alt-modular/alt-modular/Modules/cfe-bmi/include)
target_include_directories(topmodelbmi PRIVATE ../alt-modular/alt-modular/Modules/TOPMODEL/include)

#set_target_properties(cfemodel PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(cfebmi PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(topmodelbmi PROPERTIES VERSION ${PROJECT_VERSION})

#set_target_properties(cfemodel PROPERTIES PUBLIC_HEADER include/bmi_cfe.h)
set_target_properties(cfebmi PROPERTIES PUBLIC_HEADER ../alt-modular/alt-modular/Modules/cfe-bmi/include/bmi_cfe.h)
set_target_properties(topmodelbmi PROPERTIES PUBLIC_HEADER alt-modular/Modules/TOPMODEL/include/bmi_cfe.h)

include(GNUInstallDirs)

#install(TARGETS cfemodel
#        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS cfebmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS topmodelbmi
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#configure_file(cfemodel.pc.in cfemodel.pc @ONLY)
configure_file(cfebmi.pc.in cfebmi.pc @ONLY)
configure_file(topmodelbmi.pc.in topmodelbmi.pc @ONLY)

#install(FILES ${CMAKE_BINARY_DIR}/cfemodel.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
install(FILES ${CMAKE_BINARY_DIR}/cfebmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
install(FILES ${CMAKE_BINARY_DIR}/topmodelbmi.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)