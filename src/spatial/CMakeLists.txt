add_library(spatial)
add_library(NGen::spatial ALIAS spatial)

target_include_directories(spatial
    PUBLIC
        "${NGEN_INC_DIR}/spatial"
        "${NGEN_INC_DIR}/utilities"
)

target_sources(spatial
    PUBLIC
        feature.cpp
        geometry/backends/boost/boost_point.cpp
        geometry/backends/boost/boost_linestring.cpp
)
target_link_libraries(spatial PRIVATE Boost::boost)
