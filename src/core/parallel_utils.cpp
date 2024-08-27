#include <NGenConfig.h>

#include <utilities/parallel_utils.h>

int mpi_rank = 0;

#if NGEN_WITH_MPI

int mpi_num_procs = 0;

namespace parallel {

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
        MPI_Bcast(&codeBuffer, 1, MPI_UNSIGNED_SHORT, 0, MPI_COMM_WORLD);
        return codeBuffer == MPI_HF_SUB_CODE_GOOD;
    }

    bool is_hydrofabric_subdivided(int mpi_rank, int mpi_num_procs, std::string const& catchmentDataFile, bool printMsg) {
        std::string name = catchmentDataFile + "." + std::to_string(mpi_rank);
        // Initialize isGood based on local state.  Here, local file is "good" when it already exists.
        // TODO: this isn't actually checking whether the files are right (just that they are present) so do we need to?
        bool isGood = utils::FileChecker::file_is_readable(name);

        if (mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs)) {
            if (printMsg) { std::cout << "Process " << mpi_rank << ": Hydrofabric already subdivided in " << mpi_num_procs << " files." << std::endl; }
            return true;
        }
        else {
            if (printMsg) { std::cout << "Process " << mpi_rank << ": Hydrofabric has not yet been subdivided." << std::endl; }
            return false;
        }
    }

    void get_hosts_array(int mpi_rank, int mpi_num_procs, int *host_array) {
        const int ROOT_RANK = 0;
        // These are the lengths of the (trimmed) C-string representations of the hostname for each rank
        std::vector<int> actualHostnameCStrLength(mpi_num_procs);
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
        MPI_Gather(&actualHostnameCStrLength[mpi_rank], 1, MPI_INT, actualHostnameCStrLength.data(), 1, MPI_INT, ROOT_RANK,
                   MPI_COMM_WORLD);
        // Per-rank start offsets/displacements in our hostname strings gather buffer.
        std::vector<int> recvDisplacements(mpi_num_procs);
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
        MPI_Gatherv(myhostname, actualHostnameCStrLength[mpi_rank], MPI_CHAR, hostnames, actualHostnameCStrLength.data(),
                    recvDisplacements.data(), MPI_CHAR, ROOT_RANK, MPI_COMM_WORLD);

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

    bool mpi_send_text_file(const char *fileName, const int mpi_rank, const int destRank) {
        int bufSize = 4096;
        std::vector<char> buf(bufSize);
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
        while (fgets(buf.data(), bufSize, file) != NULL) {
            // Indicate we are ready to continue sending data
            MPI_Send(&continueCode, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD);

            // Send this batch
            MPI_Send(buf.data(), bufSize, MPI_CHAR, destRank, NGEN_MPI_DATA_TAG, MPI_COMM_WORLD);

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
        std::vector<char> buf(bufSize);

        int continueCode;

        while (true) {
            // Make sure the other side wants to continue sending data
            MPI_Recv(&continueCode, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (continueCode <= 0)
                break;

            MPI_Recv(buf.data(), bufSize, MPI_CHAR, srcRank, NGEN_MPI_DATA_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            writeCode = fputs(buf.data(), file);
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

    bool subdivide_hydrofabric(int mpi_rank, int mpi_num_procs, const std::string &catchmentDataFile,
                               const std::string &nexusDataFile, const std::string &partitionConfigFile)
    {
        // Track whether things are good, meaning ok to continue and, at the end, whether successful
        // Start with a value of true
        bool isGood = true;

        #if !NGEN_WITH_PYTHON
        // We can't be good to proceed with this, because Python is not active
        isGood = false;
        std::cerr << "Driver is unable to perform required hydrofabric subdividing when Python integration is not active." << std::endl;


        // Sync with the rest of the ranks and bail if any aren't ready to proceed for any reason
        if (!mpiSyncStatusAnd(isGood, mpi_rank, mpi_num_procs, "initializing hydrofabric subdivider")) {
            return false;
        }
        #else // i.e., #if NGEN_WITH_PYTHON
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
            std::vector<int> hostIdForRank(mpi_num_procs);
            get_hosts_array(mpi_rank, mpi_num_procs, hostIdForRank.data());

            // ... then (when necessary) transferring files around
            return distribute_subdivided_hydrofabric_files(catchmentDataFile, nexusDataFile, 0, mpi_rank,
                                                           mpi_num_procs, hostIdForRank.data(), true, true);

        }
        #endif // NGEN_WITH_PYTHON
    }
} // namespace parallel

#endif
