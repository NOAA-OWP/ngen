//
// Created by Robert Bartel on 1/15/26.
//

#include "gtest/gtest.h"
#if NGEN_WITH_MPI
#include "mpi.h"
#else
#include <iostream>
#endif

/** Small custom main test function for GoogleTest supporting running with MPI. */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);;

    #if NGEN_WITH_MPI
    //MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Init(&argc, &argv);
    int d = 0;
    size_t count = 0;
    //while (d == 0 && count++ < 5) {
    //    sleep(10);
    //}
    int result = RUN_ALL_TESTS();
    MPI_Finalize();
    return result;

    #else

    std::cerr << "MPI not available, skipping MPI tests." << std::endl;
    return -1;

    #endif
}