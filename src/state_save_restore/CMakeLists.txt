include(${PROJECT_SOURCE_DIR}/cmake/dynamic_sourced_library.cmake)
dynamic_sourced_cxx_library(state_save_restore "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(NGen::state_save_restore ALIAS state_save_restore)
target_link_libraries(state_save_restore PUBLIC
                           NGen::config_header
                           Boost::boost                # Headers-only Boost
                           )

#target_link_libraries(core
#        PRIVATE
#                          )

target_include_directories(state_save_restore PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        )

