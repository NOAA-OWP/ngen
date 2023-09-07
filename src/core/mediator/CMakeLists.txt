include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(core_mediator "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::core_mediator ALIAS core_mediator)

if(NGEN_WITH_UDUNITS)
    target_link_libraries(core_mediator PUBLIC libudunits2)

    # FIXME: workaround for now... there is an issue with macOS linking to
    #        the shared imported udunits2 target, such that header include
    #        directories aren't correctly propogated. - Justin
    target_include_directories(core_mediator PUBLIC "${UDUNITS2_INCLUDE}")
endif()

find_package(Threads REQUIRED)

target_include_directories(core_mediator PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/core
        ${PROJECT_SOURCE_DIR}/include/core/mediator
        )

target_link_libraries(core_mediator PUBLIC
        Boost::boost                # Headers-only Boost
        Threads::Threads
        )
