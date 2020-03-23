cmake_minimum_required(VERSION 3.10)

add_library(NGen::core_catchment STATIC
        HY_CartographicRealization.cpp,
        HY_Catchment.cpp,
        HY_CatchmentAggregate.cpp,
        HY_CatchmentArea.cpp,
        HY_CatchmentDivide.cpp,
        HY_CatchmentRealization.cpp,
        HY_CartographicRealization.cpp,
        HY_DendriticCatchment.cpp,
        HY_FlowPath.cpp,
        HY_HydroNetwork.cpp,
        HY_InteriorCatchment.cpp
        )

target_include_directories(NGen::core_catchment PUBLIC
        ../../../include/core
        ../../../include/core/catchment
        )

target_link_libraries(NGen::core_catchment PUBLIC
        Boost::boost                # Headers-only Boost
        )

# TODO: this may not be correct, need to see
add_dependencies(NGen::core_catchment NGen::core)