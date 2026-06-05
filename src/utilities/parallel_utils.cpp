#include <utilities/parallel_utils.h>

#if NGEN_WITH_MPI

#include <cstring>
#include <mpi.h>
#include <utilities/FileChecker.h>

#if NGEN_WITH_PYTHON
#include "utilities/python/HydrofabricSubsetter.hpp"
#endif // NGEN_WITH_PYTHON

namespace parallel {
    bool mpiSyncStatusAnd(bool status, MPI_Comm comm, const std::string &taskDesc) {
        int mpi_rank, mpi_num_procs;
        MPI_Comm_rank(comm, &mpi_rank);
        MPI_Comm_size(comm, &mpi_num_procs);

        // Expect 0 is good and 1 is no good for goodCode
        // TODO: assert this in constructor or somewhere, or maybe just in a unit test
        unsigned short codeBuffer;
        bool printMessage = !taskDesc.empty();
        // For the other ranks, start by properly setting the status code value in the buffer and send to rank 0
        if (mpi_rank != 0) {
            codeBuffer = status ? MPI_HF_SUB_CODE_GOOD : MPI_HF_SUB_CODE_BAD;
            MPI_Send(&codeBuffer, 1, MPI_UNSIGNED_SHORT, 0, 0, comm);
        }
        // In rank 0, the first step is to receive and process codes from the other ranks into unified global status
        else {
            for (int i = 1; i < mpi_num_procs; ++i) {
                MPI_Recv(&codeBuffer, 1, MPI_UNSIGNED_SHORT, i, 0, comm, MPI_STATUS_IGNORE);
                // If any is ever "not good", overwrite status to be "false"
                if (codeBuffer != MPI_HF_SUB_CODE_GOOD) {
                    if (printMessage) {
                        std::cerr << "Rank " << i << " not successful/ready after " << taskDesc << std::endl;
                    }
                    status = false;
                }
            }
            // Rank 0 must also now prepare the codeBuffer value for broadcasting the global status
            codeBuffer = status ? MPI_HF_SUB_CODE_GOOD : MPI_HF_SUB_CODE_BAD;
        }

        // Execute broadcast of global status rooted at rank 0
        MPI_Bcast(&codeBuffer, 1, MPI_UNSIGNED_SHORT, 0, comm);
        return codeBuffer == MPI_HF_SUB_CODE_GOOD;
    }

    bool mpiSyncStatusAnd(bool status, MPI_Comm comm) {
        return mpiSyncStatusAnd(status, comm, "");
    }

    bool is_hydrofabric_subdivided(const std::string &catchmentDataFile, MPI_Comm comm, bool printMsg) {
        int mpi_rank, mpi_num_procs;
        MPI_Comm_rank(comm, &mpi_rank);
        MPI_Comm_size(comm, &mpi_num_procs);

        std::string name = catchmentDataFile + "." + std::to_string(mpi_rank);
        // Initialize isGood based on local state.  Here, local file is "good" when it already exists.
        // TODO: this isn't actually checking whether the files are right (just that they are present) so do we need to?
        bool isGood = utils::FileChecker::file_is_readable(name);

        if (mpiSyncStatusAnd(isGood, comm)) {
            if (printMsg) {
                std::cerr << "Process " << mpi_rank << ": Hydrofabric already subdivided in " << mpi_num_procs << " files." << std::endl;
            }
            return true;
        }
        else {
            if (printMsg) {
                std::cerr << "Process " << mpi_rank << ": Hydrofabric has not yet been subdivided." << std::endl;
            }
            return false;
        }
    }

    void get_hosts_array(MPI_Comm comm, int *host_array) {
        int mpi_rank, mpi_num_procs;
        MPI_Comm_rank(comm, &mpi_rank);
        MPI_Comm_size(comm, &mpi_num_procs);

        MPI_Comm split_by_host;
        MPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, mpi_rank, MPI_INFO_NULL, &split_by_host);

        // Use the global rank of the process with rank 0 in each
        // per-host subcommunicator as the common identifier for that
        // host, and broadcast it to all processes on each host
        int host_id = mpi_rank;
        MPI_Bcast(&host_id, 1, MPI_INT, 0, split_by_host);

        // Distribute the view of which processes reside on which host globally
        MPI_Allgather(&host_id, 1, MPI_INT, host_array, 1, MPI_INT, comm);

        MPI_Comm_free(&split_by_host);
    }

    bool mpi_send_text_file(const char *fileName, MPI_Comm comm, const int destRank) {
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
            MPI_Send(&code, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, comm);
            return false;
        }

        // Send expected size to start
        MPI_Send(&bufSize, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, comm);

        // Then get back expected size to infer other side is good to go
        MPI_Recv(&code, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, comm, MPI_STATUS_IGNORE);
        if (code != bufSize) {
            // TODO: output error message
            fclose(file);
            return false;
        }
        int continueCode = 1;
        // Then while there is more of the file to read and send, read the next batch and ...
        while (fgets(buf.data(), bufSize, file) != NULL) {
            // Indicate we are ready to continue sending data
            MPI_Send(&continueCode, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, comm);

            // Send this batch
            MPI_Send(buf.data(), bufSize, MPI_CHAR, destRank, NGEN_MPI_DATA_TAG, comm);

            // Then get back a code, which will be -1 if bad and need to exit and otherwise good
            MPI_Recv(&code, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, comm, MPI_STATUS_IGNORE);
            if (code < 0) {
                // TODO: output error message
                fclose(file);
                return false;
            }
        }
        // Once there is no more file to read and send, we should stop continuing
        continueCode = 0;
        MPI_Send(&continueCode, 1, MPI_INT, destRank, NGEN_MPI_PROTOCOL_TAG, comm);
        // Expect to get back a code of 0
        MPI_Recv(&code, 1, MPI_INT, destRank, 0, comm, MPI_STATUS_IGNORE);
        fclose(file);
        return code == 0;
    }

    bool mpi_recv_text_file(const char *fileName, MPI_Comm comm, const int srcRank) {
        int bufSize, writeCode;
        // Receive expected buffer size to start
        MPI_Recv(&bufSize, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, comm, MPI_STATUS_IGNORE);

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
            MPI_Send(&bufSize, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, comm);
            return false;
        }

        // Send back the received buffer it if file opened, confirming things are good to go for transfer
        MPI_Send(&bufSize, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, comm);

        // How much has been transferred so far
        int totalNumTransferred = 0;
        std::vector<char> buf(bufSize);

        int continueCode;

        while (true) {
            // Make sure the other side wants to continue sending data
            MPI_Recv(&continueCode, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, comm, MPI_STATUS_IGNORE);
            if (continueCode <= 0)
                break;

            MPI_Recv(buf.data(), bufSize, MPI_CHAR, srcRank, NGEN_MPI_DATA_TAG, comm, MPI_STATUS_IGNORE);

            writeCode = fputs(buf.data(), file);
            MPI_Send(&writeCode, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, comm);

            if (writeCode < 0) {
                fclose(file);
                return false;
            }
        }

        fclose(file);
        MPI_Send(&continueCode, 1, MPI_INT, srcRank, NGEN_MPI_PROTOCOL_TAG, comm);
        return true;
    }

    bool distribute_subdivided_hydrofabric_files(const std::string &baseCatchmentFile, const std::string &baseNexusFile,
                                                 const int sendingRank, MPI_Comm comm,
                                                 const int *hostIdForRank, bool syncReturnStatus, bool blockAll)
    {
        int mpi_rank, mpi_num_procs;
        MPI_Comm_rank(comm, &mpi_rank);
        MPI_Comm_size(comm, &mpi_num_procs);

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
                        isGood = isGood && mpi_send_text_file(catFileToSend.c_str(), comm, otherRank);
                        isGood = isGood && mpi_send_text_file(nexFileToSend.c_str(), comm, otherRank);
                    }
                }
            }
            else {
                // For a rank not on the same host as the sending rank, receive the transmitted file
                std::string catFileToReceive = baseCatchmentFile + "." + std::to_string(mpi_rank);
                std::string nexFileToReceive = baseNexusFile + "." + std::to_string(mpi_rank);
                // Note that, unlike a bit earlier, don't need to check prior isGood in 1st receive, because not in loop
                isGood = mpi_recv_text_file(catFileToReceive.c_str(), comm, sendingRank);
                isGood = isGood && mpi_recv_text_file(nexFileToReceive.c_str(), comm, sendingRank);
            }
        }

        // Wait when appropriate
        if (blockAll) { MPI_Barrier(comm); }

        // Sync status among the ranks also, if appropriate
        if (syncReturnStatus) {
            return mpiSyncStatusAnd(isGood, comm, "distributing subdivided hydrofabric files");
        }
        // Otherwise, just return the local status value
        else {
            return isGood;
        }
    }

    bool subdivide_hydrofabric(MPI_Comm comm, const std::string &catchmentDataFile,
                               const std::string &nexusDataFile, const std::string &partitionConfigFile)
    {
        // Track whether things are good, meaning ok to continue and, at the end, whether successful
        // Start with a value of true
        bool isGood = true;

        #if !NGEN_WITH_PYTHON
        // We can't be good to proceed with this, because Python is not active
        isGood = false;
        std::cerr  << "Driver is unable to perform required hydrofabric subdividing when Python integration is not active." << std::endl;

        // Sync with the rest of the ranks and bail if any aren't ready to proceed for any reason
        if (!mpiSyncStatusAnd(isGood, comm, "initializing hydrofabric subdivider")) {
            return false;
        }
        #else // i.e., #if NGEN_WITH_PYTHON
        int mpi_rank, mpi_num_procs;
        MPI_Comm_rank(comm, &mpi_rank);
        MPI_Comm_size(comm, &mpi_num_procs);

        // Have rank 0 handle the generation task for all files/partitions
        std::unique_ptr<utils::ngenPy::HydrofabricSubsetter> subdivider;
        // Have rank 0 handle the generation task for all files/partitions
        if (mpi_rank == 0) {
            try {
                subdivider = std::make_unique<utils::ngenPy::HydrofabricSubsetter>(catchmentDataFile, nexusDataFile,
                                                                                   partitionConfigFile);
            }
            catch (const std::exception &e) {
                std::cerr  << e.what() << std::endl;
                // Set not good if the subdivider object couldn't be instantiated
                isGood = false;
            }
        }
        // Sync ranks and bail if any aren't ready to proceed for any reason
        if (!mpiSyncStatusAnd(isGood, comm, "initializing hydrofabric subdivider")) {
            return false;
        }

        if (mpi_rank == 0) {
            // Try to perform the subdividing
            try {
                isGood = subdivider->execSubdivision();
            }
            catch (const std::exception &e) {
                std::cerr  << e.what() << std::endl;
                // Set not good if the subdivider object couldn't be instantiated
                isGood = false;
            }
        }
        // Sync ranks again here on whether subdividing was successful, having them all exit at this point if not
        if (!mpiSyncStatusAnd(isGood, comm, "executing hydrofabric subdivision")) {
            return false;
        }
        // But if the subdividing went fine ...
        else {
            // ... figure out what ranks are on hosts with each other by getting an id for host of each rank
            std::vector<int> hostIdForRank(mpi_num_procs);
            get_hosts_array(comm, hostIdForRank.data());

            // ... then (when necessary) transferring files around
            return distribute_subdivided_hydrofabric_files(catchmentDataFile, nexusDataFile, 0, comm,
                                                           hostIdForRank.data(), true, true);

        }
        #endif // NGEN_WITH_PYTHON

        // This is not actually reachable, but it quiets a compiler warning
        return false;
    }

    std::vector<std::string> gather_strings(const std::vector<std::string>& local_strings, MPI_Comm comm) {
        int mpi_rank, mpi_num_procs;
        MPI_Comm_rank(comm, &mpi_rank);
        MPI_Comm_size(comm, &mpi_num_procs);

        int root_rank = 0;

        // 1. Determine local string data size
        int local_data_size = 0;
        for (const auto& s : local_strings) {
            local_data_size += s.length() + 1; // +1 for null terminator
        }

        // 2. Gather all data sizes to root
        std::vector<int> all_data_sizes(mpi_num_procs); // Only significant at root
        MPI_Gather(&local_data_size, 1, MPI_INT, all_data_sizes.data(), 1, MPI_INT, 0, comm);

        // 3. Calculate displacements and total size at root
        std::vector<int> displs(mpi_num_procs); // Only significant at root
        int total_recv_size = 0;
        if (mpi_rank == root_rank) {
            displs[0] = 0;
            total_recv_size = all_data_sizes[0];
            for (int i = 1; i < mpi_num_procs; ++i) {
                displs[i] = displs[i-1] + all_data_sizes[i-1];
                total_recv_size += all_data_sizes[i];
            }
        }

        // 4. Allocate receive buffer at root
        std::vector<char> recv_buffer; // Only significant at root
        if (mpi_rank == root_rank) {
            recv_buffer.resize(total_recv_size);
        }

        // 5. Serialize local strings
        std::vector<char> send_buffer(local_data_size);
        int current_offset = 0;
        for (const auto& s : local_strings) {
            std::copy(s.begin(), s.end(), send_buffer.begin() + current_offset);
            send_buffer[current_offset + s.length()] = '\0'; // Null terminator
            current_offset += s.length() + 1;
        }

        // 6. Perform MPI_Gatherv
        MPI_Gatherv(send_buffer.data(), local_data_size, MPI_CHAR,
                    recv_buffer.data(), all_data_sizes.data(), displs.data(), MPI_CHAR,
                    root_rank, comm);

        // 7. Deserialize at root
        std::vector<std::string> gathered_strings;
        if (mpi_rank == root_rank) {
            int current_read_offset = 0;
            for (int i = 0; i < mpi_num_procs; ++i) {
                int proc_data_size = all_data_sizes[i];
                int start_idx = displs[i];
                int end_idx = start_idx + proc_data_size;

                // Extract strings from this process's data
                int string_start = start_idx;
                for (int j = start_idx; j < end_idx; ++j) {
                    if (recv_buffer[j] == '\0') {
                        gathered_strings.emplace_back(recv_buffer.data() + string_start,
                                                      recv_buffer.data() + j);
                        string_start = j + 1; // Start of next string
                    }
                }
            }
            // gathered_strings now contains all strings
        }
        return gathered_strings;
    }

    std::vector<std::string> broadcast_strings(const std::vector<std::string>& strings, MPI_Comm comm) {
        int mpi_rank;
        MPI_Comm_rank(comm, &mpi_rank);

        int root_rank = 0;

        // 1. Broadcast the size of the vector
        int vector_size = strings.size();
        MPI_Bcast(&vector_size, 1, MPI_INT, 0, comm);

        std::vector<std::string> out_strings(vector_size);

        // 2. Broadcast the size of each individual string
        std::vector<int> string_lengths(vector_size);
        if (mpi_rank == root_rank) {
            for (int i = 0; i < vector_size; ++i) {
                string_lengths[i] = strings[i].length() + 1; // +1 for null terminator
            }
        }
        MPI_Bcast(string_lengths.data(), vector_size, MPI_INT, 0, comm);

        // 3. Broadcast each string individually
        for (int i = 0; i < vector_size; ++i) {
            std::vector<char> buffer(string_lengths[i]);
            if (mpi_rank == root_rank) {
                std::strcpy(buffer.data(), strings[i].c_str());
            }
            MPI_Bcast(buffer.data(), string_lengths[i], MPI_CHAR, 0, comm);
            out_strings[i] = buffer.data();
        }

        return out_strings;
    }
} // namespace parallel

#endif // NGEN_WITH_MPI
