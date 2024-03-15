add_library(MPIDetour SHARED "${CMAKE_CURRENT_LIST_DIR}/MPIDetour.c")
add_library(NGen::MPI_Detour ALIAS MPIDetour)
