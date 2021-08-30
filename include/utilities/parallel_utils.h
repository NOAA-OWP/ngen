#ifndef NGEN_PARALLEL_UTILS_H
#define NGEN_PARALLEL_UTILS_H

#ifdef NGEN_MPI_ACTIVE

#ifndef MPI_HF_SUB_CODE_GOOD
#define MPI_HF_SUB_CODE_GOOD 0
#endif

#ifndef MPI_HF_SUB_CODE_BAD
#define MPI_HF_SUB_CODE_BAD 1
#endif

#include <mpi.h>
#include <string>
#include "PyHydrofabricSubsetter.hpp"

using namespace std;

namespace parallel {

    /**
     * Perform an "AND" type sync across all ranks to synchronize whether all are in a 'success' or 'ready' state.
     *
     * Function should be called at the same time by all ranks to sync on some 'success' or 'ready' state for all the ranks.
     * It handles the required MPI communication and the processing of received message content.
     *
     * Function accepts a value for some boolean status property.  This initial value is applicable only to the local MPI
     * rank, with all MPI ranks having the status property and each having its own independent value.  The function has all
     * ranks (except ``0``) send their local status to rank ``0``.  Rank ``0`` then applies a Boolean AND to the statuses
     * (including its own) to produce a global status value.  The global status is then broadcast back to the other ranks.
     * Finally, the value indicated by this global status is returned.
     *
     * @param status The initial individual state for the current MPI rank.
     * @param mpi_rank The rank of the current process.
     * @param mpi_num_procs The total number of MPI processes.
     * @param taskDesc A description of the related task, used by rank 0 to print a message when included for any rank that
     *                 is not in the success/ready state.
     * @return Whether all ranks coordinating status had a success/ready status value.
     */
    bool mpiSyncStatusAnd(bool status, int mpi_rank, int mpi_num_procs, const std::string &taskDesc) {
        // Expect 0 is good and 1 is no good for goodCode
        // TODO: assert this in constructor or somewhere, or maybe just in a unit test
        unsigned short codeBuffer;
        bool printMessage = !taskDesc.empty();
        // For the other ranks, start by properly setting the status code value in the buffer and send to rank 0
        if (mpi_rank != 0) {
            codeBuffer = status ? MPI_HF_SUB_CODE_GOOD : MPI_HF_SUB_CODE_BAD;
            MPI_Send(&codeBuffer, 1, MPI_UNSIGNED_SHORT, 0, 0, MPI_COMM_WORLD);
        }
        // In rank 0, the first step is to receive and process codes from the other ranks into unified global status
        else {
            for (int i = 1; i < mpi_num_procs; ++i) {
                MPI_Recv(&codeBuffer, 1, MPI_UNSIGNED_SHORT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                // If any is ever "not good", overwrite status to be "false"
                if (codeBuffer != MPI_HF_SUB_CODE_GOOD) {
                    if (printMessage) {
                        std::cout << "Rank " << i << " not successful/ready after " << taskDesc << std::endl;
                    }
                    status = false;
                }
            }
            // Rank 0 must also now prepare the codeBuffer value for broadcasting the global status
            codeBuffer = status ? MPI_HF_SUB_CODE_GOOD : MPI_HF_SUB_CODE_BAD;
        }
        // Execute broadcast of global status rooted at rank 0
        MPI_Bcast(&codeBuffer, mpi_num_procs - 1, MPI_UNSIGNED_SHORT, 0, MPI_COMM_WORLD);
        return codeBuffer == MPI_HF_SUB_CODE_GOOD;
    }

    /**
     * Convenience method for overloaded function when no message is needed, and thus no description param provided.
     *
     * @param status The initial individual state for the current MPI rank.
     * @param mpi_rank The rank of the current process.
     * @param mpi_num_procs The total number of MPI processes.
     * @return Whether all ranks coordinating status had a success/ready status value.
     * @see mpiSyncStatusAnd(bool, const std::string&)
     */
    bool mpiSyncStatusAnd(bool status, int mpi_rank, int mpi_num_procs) {
        return mpiSyncStatusAnd(status, mpi_rank, mpi_num_procs, "");
    }

    /**
     * Check whether the parameter hydrofabric files have been subdivided into appropriate per partition files.
     *
     * Checks if partition specific subfiles corresponding to each partition/process already exist.  E.g., for a file at
     * ``/dirname/catchment_data.geojson`` and two MPI processes, checks if both ``/dirname/catchment_data.geojson.0``
     * and ``/dirname/catchment_data.geojson.1 `` exist.
     *
     * This check is performed for both the catchment and nexus hydrofabric base file names, as stored in the global
     * ``catchmentDataFile`` and ``nexusDataFile`` variables respectively.  The number of MPI processes is obtained from
     * the global ``mpi_rank`` variable.
     *
     * @param mpi_rank The rank of the current process.
     * @param mpi_num_procs The total number of MPI processes.
     * @param printMessage Whether a supplemental message should be printed to standard out indicating status.
     *
     * @return Whether proprocessing has already been performed to divide the main hydrofabric into existing, individual
     *         sub-hydrofabric files for each partition/process.
     */
    bool is_hydrofabric_subdivided(int mpi_rank, int mpi_num_procs, bool printMsg) {
        std::string name = catchmentDataFile + "." + std::to_string(mpi_rank);
        // Initialize isGood based on local state.  Here, local file is "good" when it already exists.
        // TODO: this isn't actually checking whether the files are right (just that they are present) so do we need to?
        bool isGood = utils::FileChecker::file_is_readable(name);
        if (mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs)) {
            if (printMsg) { std::cout << "Hydrofabric already subdivided in " << mpi_num_procs << " files." << std::endl; }
            return true;
        }
        else {
            if (printMsg) { std::cout << "Hydrofabric has not yet been subdivided." << std::endl; }
            return false;
        }
    }

    /**
     * Convenience overloaded method for when no supplemental output message is required.
     *
     * @param mpi_rank The rank of the current process.
     * @param mpi_num_procs The total number of MPI processes.
     * @return Whether proprocessing has already been performed to divide the main hydrofabric into existing, individual
     *         sub-hydrofabric files for each partition/process.
     * @see is_hydrofabric_subdivided(bool)
     */
    bool is_hydrofabric_subdivided(int mpi_rank, int mpi_num_procs) {
        return is_hydrofabric_subdivided(mpi_rank, mpi_num_procs, false);
    }

    /**
     * Attempt to subdivide the passed hydrofabric files into a series of per-partition files.
     *
     * This function assumes that, when it is called, the intent is for it to produce a freshly subdivided hydrofabric
     * and associated files.  As a result, if there are any other subdivided hydrofabric files present having the same
     * names as the files the function will write, then those preexisting files are considered stale and overwritten.
     *
     * @param mpi_rank The rank of the current process.
     * @param mpi_num_procs The total number of MPI processes.
     * @param catchmentDataFile The path to the catchment data file for the hydrofabric.
     * @param nexusDataFile The path to the nexus data file for the hydrofabric.
     * @param partitionConfigFile The path to distributed processing hydrofabric partitioning config.
     * @return Whether subdividing was successful.
     */
    bool subdivide_hydrofabric(int mpi_rank, int mpi_num_procs, const string &catchmentDataFile,
                               const string &nexusDataFile, const string &partitionConfigFile)
    {
        // Track whether things are good, meaning ok to continue and, at the end, whether successful
        // Start with a value of true
        bool isGood = true;

        // For now just have this be responsible for its own rank file
        // Later consider whether it makes more sense for one rank (per host) to write all files
        std::unique_ptr<utils::PyHydrofabricSubsetter> subdivider;
        try {
            subdivider = std::make_unique<utils::PyHydrofabricSubsetter>(catchmentDataFile, nexusDataFile,
                                                                         partitionConfigFile);
        }
        catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            // Set not good if the subdivider object couldn't be instantiated
            isGood = false;
        }
        // Sync with the rest of the ranks and bail if any aren't ready to proceed for any reason
        if (!mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs, "initializing hydrofabric subdivider")) {
            return false;
        }

        // Try to perform the subdividing (for now, have each rank handle its own file)
        try {
            isGood = subdivider->execSubdivision(mpi_rank);
        }
        catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            // Set not good if the subdivider object couldn't be instantiated
            isGood = false;
        }

        // Now sync ranks on whether the subdividing function was executed successfully, and return
        return mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs, "executing hydrofabric subdivision");
    }
}

#endif // NGEN_MPI_ACTIVE

#endif //NGEN_PARALLEL_UTILS_H
