cmake_minimum_required(VERSION 3.10)

include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(forcing "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::forcing ALIAS forcing)
target_include_directories(forcing PUBLIC
        ${PROJECT_SOURCE_DIR}/include/forcing
        )

#target_link_libraries(forcing PUBLIC)


#find_package(Boost)
#target_link_libraries(forcing PUBLIC
#        Boost::boost                # Headers-only Boost
#        )
