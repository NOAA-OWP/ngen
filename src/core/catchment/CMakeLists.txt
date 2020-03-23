cmake_minimum_required(VERSION 3.10)

add_library(core_catchment STATIC
        HY_CartographicRealization.cpp
        HY_Catchment.cpp
        HY_CatchmentAggregate.cpp
        HY_CatchmentArea.cpp
        HY_CatchmentDivide.cpp
        HY_CatchmentRealization.cpp
        HY_CartographicRealization.cpp
        HY_DendriticCatchment.cpp
        HY_FlowPath.cpp
        HY_HydroNetwork.cpp
        HY_InteriorCatchment.cpp
        )

add_library(NGen::core_catchment ALIAS core_catchment)

target_include_directories(core_catchment PUBLIC
        ../../../include/core
        ../../../include/core/catchment
        )

# TODO: consider setting a minimum or required version
find_package(Boost)

target_link_libraries(core_catchment PUBLIC
        Boost::boost                # Headers-only Boost
        )

# TODO: this may not be correct, need to see
add_dependencies(core_catchment NGen::core)