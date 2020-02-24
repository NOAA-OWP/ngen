cmake_minimum_required(VERSION 3.10)
########### Define project version and use via generated config header
project(ngen VERSION 0.1.0)
configure_file(include/NGenConfig.h.in include/NGenConfig.h)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(APP_INCLUDE_DIR ./include)
set(APP_MODELS_DIR ./models)

set(HYMOD_DIR ${APP_MODELS_DIR}/hymod)
set(HYMOD_INCLUDE_DIR ${HYMOD_DIR}/include)

include_directories(${APP_INCLUDE_DIR})
include_directories(${HYMOD_INCLUDE_DIR})

add_executable(ngen src/core/NGen.cpp)

target_include_directories(ngen PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${HYMOD_INCLUDE_DIR}"
        )

#add_library(Hymod ${HYMOD_INCLUDE_DIR}/Hymod.h)
#set_target_properties(Hymod PROPERTIES LINKER_LANGUAGE CXX)

#target_compile_features(Hymod PUBLIC cxx_std_14)

#add_executable(ngen
#        include/GM_Object.h
#        include/GridPolygon.h
#        include/HY_CartographicRealization.h
#        include/HY_CatchementDivide.h
#        include/HY_Catchment.h
#        include/HY_CatchmentAggregate.h
#        include/HY_CatchmentArea.h
#        include/HY_CatchmentDivide.h
#        include/HY_CatchmentRealization.h
#        include/HY_DendriticCatchment.h
#        include/HY_DistanceDiscription.h
#        include/HY_FlowPath.h
#        include/HY_HydroFeature.h
#        include/HY_HydroLocationType.h
#        include/HY_HydroNetwork.h
#        include/HY_HydroNexus.h
#        include/HY_InteriorCatchment.h
#        include/HY_PointHydroNexus.h
#        include/LinearReservoir.h
#        include/Simple_Lumped_Model_Realization.h
#        models/hymod/include/Hymod.h)
#
#set_target_properties(ngen PROPERTIES LINKER_LANGUAGE CXX)
#
#target_compile_options(ngen PUBLIC -std=c++14 -Wall)
#target_compile_features(ngen PUBLIC cxx_std_14)
