add_library(ngen_parallel)
add_library(NGen::parallel ALIAS ngen_parallel)

target_include_directories(ngen_parallel
  PUBLIC
    ${NGEN_INC_DIR}
)

target_link_libraries(ngen_parallel
  PUBLIC
    NGen::config_header
    NGen::logging
)

if(NGEN_WITH_MPI)
  target_link_libraries(ngen_parallel
    PUBLIC
      MPI::MPI_CXX
      Boost::boost
  )
  if(NGEN_WITH_PYTHON)
    target_link_libraries(ngen_parallel
      PUBLIC
        pybind11::embed
        NGen::python
    )
  endif()
endif()

target_sources(ngen_parallel
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/parallel_utils.cpp"
)

add_library(ngen_memfd_file_buffer)
add_library(NGen::memfd_file_buffer ALIAS ngen_memfd_file_buffer)

target_include_directories(ngen_memfd_file_buffer
  PUBLIC
    ${NGEN_INC_DIR}
)

target_sources(ngen_memfd_file_buffer
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/MemfdFileBuffer.cpp"
)
