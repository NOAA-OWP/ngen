#ifndef NGEN_PARALLEL_UTILS_H
#define NGEN_PARALLEL_UTILS_H

#ifdef NGEN_MPI_ACTIVE

#ifndef MPI_HF_SUB_CODE_GOOD
#define MPI_HF_SUB_CODE_GOOD 0
#endif

#ifndef MPI_HF_SUB_CODE_BAD
#define MPI_HF_SUB_CODE_BAD 1
#endif

#ifndef NGEN_MPI_DATA_TAG
#define NGEN_MPI_DATA_TAG 100
#endif

#ifndef NGEN_MPI_PROTOCOL_TAG
#define NGEN_MPI_PROTOCOL_TAG 101
#endif

#include <cstring>
#include <mpi.h>
#include <string>
#include <set>
#ifdef ACTIVATE_PYTHON
#include "python/HydrofabricSubsetter.hpp"
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
        const int ROOT_RANK = 0;
        // These are the lengths of the (trimmed) C-string representations of the hostname for each rank
        int actualHostnameCStrLength[mpi_num_procs];
        // Initialize to -1 to represent unknown
        for (int i = 0; i < mpi_num_procs; ++i) {
            actualHostnameCStrLength[i] = -1;
        }

        // Get this rank's hostname (things should never be longer than 256)
        char myhostname[256];
        gethostname(myhostname, 256);

        // Set the one for this rank
        actualHostnameCStrLength[mpi_rank] = std::strlen(myhostname);

        // First, gather the hostname string lengths
        MPI_Gather(&actualHostnameCStrLength[mpi_rank], 1, MPI_INT, actualHostnameCStrLength, 1, MPI_INT, ROOT_RANK,
                   MPI_COMM_WORLD);
        // Per-rank start offsets/displacements in our hostname strings gather buffer.
        int recvDisplacements[mpi_num_procs];
        int totalLength = 0;
        for (int i = 0; i < mpi_num_procs; ++i) {
            // Displace each rank's string by the sum of the length of the previous rank strings
            recvDisplacements[i] = totalLength;
            // Adding the extra to make space for the \0
            actualHostnameCStrLength[i] += 1;
            // Then update the total length
            totalLength += actualHostnameCStrLength[i];

        }
        // Now we can create our buffer array and gather the hostname strings into it
        char hostnames[totalLength];
        MPI_Gatherv(myhostname, actualHostnameCStrLength[mpi_rank], MPI_CHAR, hostnames, actualHostnameCStrLength,
                    recvDisplacements, MPI_CHAR, ROOT_RANK, MPI_COMM_WORLD);

        if (mpi_rank == ROOT_RANK) {
            host_array[0] = 0;
            int next_host_id = 1;

            int rank_with_matching_hostname;
            char *checked_rank_hostname, *known_rank_hostname;

            for (int rank_being_check = 0; rank_being_check < mpi_num_procs; ++rank_being_check) {
                // Set this as negative initially for each rank check to indicate no match found (at least yet)
                rank_with_matching_hostname = -1;
                // Get a C-string pointer for this rank's hostname, offset by the appropriate displacement
                checked_rank_hostname = &hostnames[recvDisplacements[rank_being_check]];

                // Assume that hostnames for any ranks less than the current rank being check are already known
                for (int known_rank = 0; known_rank < rank_being_check; ++known_rank) {
                    // Get the right C-string pointer for the current known rank's hostname also
                    known_rank_hostname = &hostnames[recvDisplacements[known_rank]];
                    // Compare the hostnames, setting and breaking if a match is found
                    if (std::strcmp(known_rank_hostname, checked_rank_hostname) == 0) {
                        rank_with_matching_hostname = known_rank;
                        break;
                    }
                }
                // This indicates this rank had no earlier rank with a matching hostname.
                if (rank_with_matching_hostname < 0) {
                    // Assign new host id, then increment what the next id will be
                    host_array[rank_being_check] = next_host_id++;
                }
                else {
                    host_array[rank_being_check] = host_array[rank_with_matching_hostname];
                }
            }
        }
        // Now, broadcast the results out
        MPI_Bcast(host_array, mpi_num_procs, MPI_INT, 0, MPI_COMM_WORLD);
    }

    /**
     * Send the contents of a text file to another MPI rank.
     *
     * Note that the file is read with ``fgets``.
     *
     * @param fileName The text file to read and send its contents.
     * @param mpi_rank The current MPI rank.
     * @param destRank The MPI rank to which the file data should be sent.
     * @return Whether sending was successful.
     */
    bool mpi_send_text_file(const char *fileName, const int mpi_rank, const int destRank) {
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
            MPI_Send(&code, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD);
            return false;
        }

        // Send expected size to start
        MPI_Send(&bufSize, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD);

        // Then get back expected size to infer other side is good to go
        MPI_Recv(&code, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (code != bufSize) {
            // TODO: output error message
            fclose(file);
            return false;
        }
        int continueCode = 1;
        // Then while there is more of the file to read and send, read the next batch and ...
        while (fgets(buf, bufSize, file) != NULL) {
            // Indicate we are ready to continue sending data
            MPI_Send(&continueCode, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD);

            // Send this batch
            MPI_Send(buf, bufSize, MPI_CHAR, destRank, NGEN_MPI_DATA_TAG, MPI_COMM_WORLD);

            // Then get back a code, which will be -1 if bad and need to exit and otherwise good
            MPI_Recv(&code, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (code < 0) {
                // TODO: output error message
                fclose(file);
                return false;
            }
        }
        // Once there is no more file to read and send, we should stop continuing
        continueCode = 0;
        MPI_Send(&continueCode, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD);
        // Expect to get back a code of 0
        MPI_Recv(&code, 1, MPI_INT, destRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        fclose(file);
        return code == 0;
    }

    /**
     * Receive text data from another MPI rank and write the contents to a text file.
     *
     * Files are created if necessary and will be overwritten if they exist.
     *
     * Note that the file is written with ``fputs``.
     *
     * @param fileName The text file to which data should be written.
     * @param mpi_rank The current MPI rank.
     * @param destRank The MPI rank to which the file data should be sent.
     * @return Whether sending was successful.
     */
    bool mpi_recv_text_file(const char *fileName, const int mpi_rank, const int srcRank) {
        int bufSize, writeCode;
        // Receive expected buffer size to start
        MPI_Recv(&bufSize, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

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
            MPI_Send(&bufSize, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD);
            return false;
        }

        // Send back the received buffer it if file opened, confirming things are good to go for transfer
        MPI_Send(&bufSize, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD);

        // How much has been transferred so far
        int totalNumTransferred = 0;
        char buf[bufSize];

        int continueCode;

        while (true) {
            // Make sure the other side wants to continue sending data
            MPI_Recv(&continueCode, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (continueCode <= 0)
                break;

            MPI_Recv(buf, bufSize, MPI_CHAR, srcRank, NGEN_MPI_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            writeCode = fputs(buf, file);
            MPI_Send(&writeCode, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD);

            if (writeCode < 0) {
                fclose(file);
                return false;
            }
        }

        fclose(file);
        MPI_Send(&continueCode, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD);
        return true;
    }


    /**
     * Distribute subdivided hydrofabric files to ranks on other hosts as needed.
     *
     * Distribute subdivided hydrofabric files among ranks as needed.  This is optimized by receiving a "sending"
     * rank parameter, only sending to files to ranks not on the same host as the sending rank, and only sending a rank
     * its specific set of subdivided files.
     *
     * Host identification is provided in a relative fashion via an array param that maps ranks to hosts, using
     * arbitrary but consistent identifier
     *
     * The function assumes that rank-specific files will be in an analogous directory with the same path across hosts.
     * It does not, however, perform any actions to create such directories.  Additionally, it assumes the rank-
     * specific files will have a name derived from the base file name, but with ``.`` and the rank appended as a
     * suffix.  Finally, it assumes that the sending rank has all the necessary rank-specific files to be distributed
     * and that those existing on the sending rank are appropriate for distribution (and potentially replacing existing
     * files on other hosts).
     *
     * Execution Details
     * ------------------------
     * Depending on the sending rank and whether the current rank either is, or is on the same host as, the sending
     * rank, one of three things will occur as part of this function:
     *
     *  * execution of a sending logic block (the sending rank only)
     *  * execution of a receiving logic block (any destination rank; i.e., rank on a host different from sending rank)
     *  * bypassing of both communication logic blocks (ranks on same host as sending rank, except sending rank itself)
     *
     * The sending logic block loops over rank id values. If an id is for a destination rank, @see mpi_send_text_file
     * is used to send that destination rank its files.  Similarly, the receiving logic block uses
     * @see mpi_recv_text_file to receive its files, overwriting any that already exist.
     *
     * Optionally, an MPI barrier may be added after the execution/bypassing of the communication logic.
     *
     * During the communication logic, a status value is maintained for whether ALL executed communication actions have
     * been successful.  This value is initially set to ``true``.  Optionally, after the communication logic,
     * the function can sync this status across all ranks, setting every rank's status to ``false`` unless all of them
     * are ``true`` prior to the sync.  Finally, this status value is returned.
     *
     * @param baseCatchmentFile The base catchment data file from which the rank-specific files are derived.
     * @param baseNexusFile The base nexus data file from which the rank-specific files are derived.
     * @param sendingRank The rank that will be the source and send data to ranks on different hosts.
     * @param mpi_rank The rank of the current process.
     * @param mpi_num_procs The number of MPI ranks.
     * @param hostIdForRank Pointer to array of size ``mpi_num_procs`` that maps ranks (index) to hosts, with each
     *                      distinct host having some identifier value uniquely representing it within the array
     *                      (e.g., if hostIdForRank[0] == hostIdForRank[1] then rank 0 and rank 1 are on the same host).
     * @param syncReturnStatus Whether all ranks should AND-sync their return status before returning.
     * @param blockAll Whether an MPI barrier should be added before (when appropriate) syncing status and returning.
     * @return
     */
    bool distribute_subdivided_hydrofabric_files(const std::string &baseCatchmentFile, const std::string &baseNexusFile,
                                                 const int sendingRank, const int mpi_rank, const int mpi_num_procs,
                                                 const int *hostIdForRank, bool syncReturnStatus, bool blockAll)
    {
        // Start with status as good
        bool isGood = true;
        // FIXME: For now, just have rank 0 send everything, but optimize with multiple procs or threads later
        // Only need to process this if sending rank or a receiving ranks (i.e., not on same host as sending rank)
        if (mpi_rank == sendingRank || hostIdForRank[mpi_rank] != hostIdForRank[sendingRank]) {
            // Have the sending rank send out all files
            if (mpi_rank == sendingRank) {
                // In rank 0, for all the other ranks ...
                for (int otherRank = 0; otherRank < mpi_num_procs; ++otherRank) {
                    // If another rank is on a different host (note that this covers otherRank == sendingRank case) ...
                    if (hostIdForRank[otherRank] != hostIdForRank[mpi_rank]) {
                        // ... then send that rank its rank-specific catchment and nexus files
                        std::string catFileToSend = baseCatchmentFile + "." + std::to_string(otherRank);
                        std::string nexFileToSend = baseNexusFile + "." + std::to_string(otherRank);
                        // Note that checking previous isGood is necessary here because of loop
                        isGood = isGood && mpi_send_text_file(catFileToSend.c_str(), mpi_rank, otherRank);
                        isGood = isGood && mpi_send_text_file(nexFileToSend.c_str(), mpi_rank, otherRank);
                    }
                }
            }
            else {
                // For a rank not on the same host as the sending rank, receive the transmitted file
                std::string catFileToReceive = baseCatchmentFile + "." + std::to_string(mpi_rank);
                std::string nexFileToReceive = baseNexusFile + "." + std::to_string(mpi_rank);
                // Note that, unlike a bit earlier, don't need to check prior isGood in 1st receive, because not in loop
                isGood = mpi_recv_text_file(catFileToReceive.c_str(), mpi_rank, sendingRank);
                isGood = isGood && mpi_recv_text_file(nexFileToReceive.c_str(), mpi_rank, sendingRank);
            }
        }

        // Wait when appropriate
        if (blockAll) { MPI_Barrier(MPI_COMM_WORLD); }

        // Sync status among the ranks also, if appropriate
        if (syncReturnStatus) {
            return mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs, "distributing subdivided hydrofabric files");
        }
        // Otherwise, just return the local status value
        else {
            return isGood;
        }
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

        #ifndef ACTIVATE_PYTHON
        // We can't be good to proceed with this, because Python is not active
        isGood = false;
        std::cerr << "Driver is unable to perform required hydrofabric subdividing when Python integration is not active." << std::endl;


        // Sync with the rest of the ranks and bail if any aren't ready to proceed for any reason
        if (!mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs, "initializing hydrofabric subdivider")) {
            return false;
        }
        #else // i.e., #ifdef ACTIVATE_PYTHON
        // Have rank 0 handle the generation task for all files/partitions
        std::unique_ptr<utils::ngenPy::HydrofabricSubsetter> subdivider;
        // Have rank 0 handle the generation task for all files/partitions
        if (mpi_rank == 0) {
            try {
                subdivider = std::make_unique<utils::ngenPy::HydrofabricSubsetter>(catchmentDataFile, nexusDataFile,
                                                                                   partitionConfigFile);
            }
            catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
                // Set not good if the subdivider object couldn't be instantiated
                isGood = false;
            }
        }
        // Sync ranks and bail if any aren't ready to proceed for any reason
        if (!mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs, "initializing hydrofabric subdivider")) {
            return false;
        }

        if (mpi_rank == 0) {
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
        // Sync ranks again here on whether subdividing was successful, having them all exit at this point if not
        if (!mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs, "executing hydrofabric subdivision")) {
            return false;
        }
        // But if the subdividing went fine ...
        else {
            // ... figure out what ranks are on hosts with each other by getting an id for host of each rank
            int hostIdForRank[mpi_num_procs];
            get_hosts_array(mpi_rank, mpi_num_procs, hostIdForRank);

            // ... then (when necessary) transferring files around
            return distribute_subdivided_hydrofabric_files(catchmentDataFile, nexusDataFile, 0, mpi_rank,
                                                           mpi_num_procs, hostIdForRank, true, true);

        }
        #endif // ACTIVATE_PYTHON
    }
}

#endif // NGEN_MPI_ACTIVE

#endif //NGEN_PARALLEL_UTILS_H
