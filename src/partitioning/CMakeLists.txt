add_library(ngen_partitioning)
add_library(NGen::partitioning ALIAS ngen_partitioning)

target_include_directories(ngen_partitioning
  PUBLIC
    ${NGEN_INC_DIR}
)

target_link_libraries(ngen_partitioning
  PUBLIC
    NGen::geojson
)

target_sources(ngen_partitioning
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/Partition_By_Nexus.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/Strategy_Nexus_Count.cpp"
)

