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

add_library(ngen_multithreading)
add_library(NGen::multithreading ALIAS ngen_multithreading)

find_package(Threads REQUIRED)

target_include_directories(ngen_multithreading
  PUBLIC
    ${NGEN_INC_DIR}
)

target_link_libraries(ngen_multithreading
  PUBLIC
    Threads::Threads
)

if(NGEN_WITH_OPENMP)
  target_link_libraries(ngen_multithreading PUBLIC OpenMP::OpenMP_CXX)
  # Workaround for CMake bug #21818 that it will not automatically add the linking flags
  target_link_options(ngen_multithreading PUBLIC "-fopenmp")
endif()

target_sources(ngen_multithreading
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/multithreading.cpp"
)
