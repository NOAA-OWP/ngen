#ifndef NGEN_PARALLEL_UTILS_H
#define NGEN_PARALLEL_UTILS_H

#ifdef NGEN_MPI_ACTIVE

#ifndef MPI_HF_SUB_CODE_GOOD
#define MPI_HF_SUB_CODE_GOOD 0
#endif

#ifndef MPI_HF_SUB_CODE_BAD
#define MPI_HF_SUB_CODE_BAD 1
#endif

#include <cstring>
#include <mpi.h>
#include <string>
#include <set>
#ifdef ACTIVATE_PYTHON
#include "PyHydrofabricSubsetter.hpp"
#endif // ACTIVATE_PYTHON

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

    bool distribute_subdivided_hydrofabric_files() {

    }

    /**
     * Set each rank's host "id" value in a provided host array.
     *
     * This function has the ranks communicate their hostname back to rank 0.  Rank 0 then builds an array of unique
     * host id values.  The value at index ``x`` in this array is the id for the host of rank ``x``.
     *
     * The id of the host of rank 0 is ``0``, but otherwise there is no deterministic ordering of id values for hosts.
     *
     * This array is then broadcast by rank 0 back to to the other ranks, allowing them to set their analogous arrays.
     * It is expected all ranks run this function at the same time.
     *
     * @param mpi_rank The current rank.
     * @param mpi_num_procs The number of ranks.
     * @param host_array A pointer to an allocated array of size ``mpi_num_procs``.
     */
    void get_hosts_array(int mpi_rank, int mpi_num_procs, int *host_array) {
        // Things should never be longer than this
        int hostnameBufferSize = 256;
        char hostnames[mpi_num_procs][hostnameBufferSize];
        // These are the lengths of the (trimmed) C-string representations of the hostname for each rank
        int actualHostnameCStrLength[mpi_num_procs];
        // Initialize to -1 to represent unknown
        for (int i = 0; i < mpi_num_procs; ++i) {
            actualHostnameCStrLength[i] = -1;
        }

        // Get this rank's hostname
        gethostname(hostnames[mpi_rank], hostnameBufferSize);
        // Set the one for this rank
        actualHostnameCStrLength[mpi_rank] = std::strlen(hostnames[mpi_rank]);

        if (mpi_rank != 0) {
            // First, send how long the string is
            MPI_Send(&actualHostnameCStrLength[mpi_rank], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(hostnames[mpi_rank], actualHostnameCStrLength[mpi_rank], MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
        // In rank 0, get the other hostnames
        else {
            host_array[0] = 0;
            int next_host_id = 1;

            for (int recv_rank = 1; recv_rank < mpi_num_procs; ++recv_rank) {
                MPI_Recv(&actualHostnameCStrLength[recv_rank], 1, MPI_INT, recv_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(hostnames[recv_rank], actualHostnameCStrLength[recv_rank], MPI_CHAR, recv_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                int matches_host_at = -1;

                for (int known_rank = 0; known_rank < recv_rank; ++known_rank) {
                    if (std::strcmp(hostnames[known_rank], hostnames[recv_rank]) == 0) {
                        matches_host_at = known_rank;
                        break;
                    }
                }
                if (matches_host_at == -1) {
                    // Assign new host id, then increment what the next id will be
                    host_array[recv_rank] = next_host_id++;
                }
                else {
                    host_array[recv_rank] = host_array[matches_host_at];
                }
            }
        }
        // Now, broadcast the results out
        MPI_Bcast(host_array, mpi_num_procs, MPI_INT, 0, MPI_COMM_WORLD);
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

        #ifdef ACTIVATE_PYTHON
        // Have rank 0 handle the generation task for all files/partitions
        if (mpi_rank == 0) {
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

            // Try to perform the subdividing
            try {
                isGood = subdivider->execSubdivision();
            }
            catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
                // Set not good if the subdivider object couldn't be instantiated
                isGood = false;
            }
        }

        #else // i.e., ifndef ACTIVATE_PYTHON
        // Without Python available, there is no way external subdivide package can be used (i.e., not good)
        isGood = false;
        std::cerr << "Driver is unable to perform required hydrofabric subdividing when Python integration is not active." << std::endl;
        #endif // ACTIVATE_PYTHON

        // Now sync ranks on whether the subdividing function was executed successfully
        // If not, meaning rank 0 couldn't do subdividing, then have all ranks exit at this point
        if (!mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs, "executing hydrofabric subdivision")) {
            return false;
        }
        // But if the subdividing went fine, then we need to work out (possibly) transferring files around
        else {
            // Figure out which ranks are and are not on the same host as rank 0
            int hostArray[mpi_num_procs];
            get_hosts_array(mpi_rank, mpi_num_procs, hostArray);
            // FIXME: For now, just have rank 0 send everything, but optimize with multiple procs or threads later
            // Only the transmitting (i.e., rank 0) and receiving ranks (not the same host as rank 0) need to do this
            if (mpi_rank == 0 || hostArray[mpi_rank] != hostArray[0]) {
                int bufSize = 4096;
                char buf[bufSize];

                if (mpi_rank == 0) {
                    for (int i = 1; i < mpi_num_procs; ++i) {
                        if (hostArray[i] != hostArray[0]) {
                            // TODO: transmit from rank 0 to rank i the i-th catchment and nexus files
                            asdf
                        }
                    }
                }
                else {
                    // TODO: receive the transmitted file
                }
            }

            // TODO: barrier for all

        }
    }
}

bool mpi_send_file(char *fileName, int mpi_rank, int destRank) {
    int bufSize = 4096;
    char buf[bufSize];
    int code;
    // How much has been transferred so far
    int totalNumTransferred = 0;

    FILE *file = fopen(fileName, "r");

    // Transmit error code instead of expected size and return false if file can't be opened
    if (file == NULL) {
        // TODO: output error message
        code = -1;
        MPI_Send(&code, 1, MPI_INT, destRank, 0, MPI_COMM_WORLD);
        return false;
    }

    // Send expected size to start
    MPI_Send(&bufSize, 1, MPI_INT, destRank, 0, MPI_COMM_WORLD);

    // Then get back expected size to infer other side is good to go
    MPI_Recv(&code, 1, MPI_INT, destRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (code != bufSize) {
        // TODO: output error message
        fclose(file);
        return false;
    }

    // Then while there is more of the file to read and send, read the next batch and ...
    while (fgets(buf, bufSize, file) != NULL) {
        // First send this batch
        MPI_Send(buf, bufSize, MPI_CHAR, destRank, 0, MPI_COMM_WORLD);
        // Then get back a code, which will be -1 if bad and need to exit and otherwise good
        MPI_Recv(&code, 1, MPI_INT, destRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (code == -1) {
            // TODO: output error message
            fclose(file);
            return false;
        }
    }
    // Once there is no more file to read and send, send an empty string
    MPI_Send("", bufSize, MPI_CHAR, destRank, 0, MPI_COMM_WORLD);
    // Expect to get back a code of 0
    MPI_Recv(&code, 1, MPI_INT, destRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    fclose(file);
    return code == 0;
}

bool mpi_recv_file(char *fileName, int mpi_rank, int srcRank) {
    int bufSize, strLength, writeCode;
    // Receive expected buffer size to start
    MPI_Recv(&bufSize, 1, MPI_INT, srcRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // If the sending side couldn't open the file, then immediately return false
    if (bufSize == -1) {
        // TODO: output error
        return false;
    }

    // Try to open recv file ...
    FILE *file = fopen(fileName, "w");
    // ... and let sending size know whether this was successful by sending error code if not ...
    if (file == NULL) {
        // TODO: output error message
        bufSize = -1;
        MPI_Send(&bufSize, 1, MPI_INT, srcRank, 0, MPI_COMM_WORLD);
        return false;
    }

    // Send back the received buffer it if file opened, confirming things are good to go for transfer
    MPI_Send(&bufSize, 1, MPI_INT, srcRank, 0, MPI_COMM_WORLD);

    // How much has been transferred so far
    int totalNumTransferred = 0;
    char buf[bufSize];

    // While the last received string batch contained something
    while (true) {
        MPI_Recv(buf, bufSize, MPI_CHAR, srcRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        strLength = std::strlen(buf);
        // As long as the
        if (strLength > 0) {

            // Write to file
            writeCode = fputs(buf, file);
            if (writeCode >= 0) {
                MPI_Send(&strLength, 1, MPI_INT, srcRank, 0, MPI_COMM_WORLD);
            }
            else {
                MPI_Send(&writeCode, 1, MPI_INT, srcRank, 0, MPI_COMM_WORLD);
                fclose(file);
                return false;
            }
        }
        else {
            // When finished ...
            fclose(file);
            // Also send back understood finished code of 0
            MPI_Send(&strLength, 1, MPI_INT, srcRank, 0, MPI_COMM_WORLD);
            break;
        }
    }

    return true;
}

#endif // NGEN_MPI_ACTIVE

#endif //NGEN_PARALLEL_UTILS_H
