#include <utilities/parallel_utils.h>

#if NGEN_WITH_MPI

#include <climits>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <mpi.h>

namespace parallel {
    namespace {
        //! Codes each rank sends to say whether it is in a good state, for mpiSyncStatusAnd()
        constexpr unsigned short STATUS_GOOD = 0;
        constexpr unsigned short STATUS_BAD = 1;
    }

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
            codeBuffer = status ? STATUS_GOOD : STATUS_BAD;
            MPI_Send(&codeBuffer, 1, MPI_UNSIGNED_SHORT, 0, 0, comm);
        }
        // In rank 0, the first step is to receive and process codes from the other ranks into unified global status
        else {
            for (int i = 1; i < mpi_num_procs; ++i) {
                MPI_Recv(&codeBuffer, 1, MPI_UNSIGNED_SHORT, i, 0, comm, MPI_STATUS_IGNORE);
                // If any is ever "not good", overwrite status to be "false"
                if (codeBuffer != STATUS_GOOD) {
                    if (printMessage) {
                        std::cerr << "Rank " << i << " not successful/ready after " << taskDesc << std::endl;
                    }
                    status = false;
                }
            }
            // Rank 0 must also now prepare the codeBuffer value for broadcasting the global status
            codeBuffer = status ? STATUS_GOOD : STATUS_BAD;
        }

        // Execute broadcast of global status rooted at rank 0
        MPI_Bcast(&codeBuffer, 1, MPI_UNSIGNED_SHORT, 0, comm);
        return codeBuffer == STATUS_GOOD;
    }

    bool mpiSyncStatusAnd(bool status, MPI_Comm comm) {
        return mpiSyncStatusAnd(status, comm, "");
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

    std::vector<std::string> scatter_strings(const std::vector<std::vector<std::string>>& per_rank, int root, MPI_Comm comm) {
        int mpi_rank, mpi_num_procs;
        MPI_Comm_rank(comm, &mpi_rank);
        MPI_Comm_size(comm, &mpi_num_procs);

        // At root, pack each rank's strings, NUL-terminated, into consecutive spans of one buffer
        std::vector<int> send_sizes;
        std::vector<int> displs;
        std::vector<char> send_buffer;
        if (mpi_rank == root) {
            if (per_rank.size() != static_cast<std::size_t>(mpi_num_procs)) {
                throw std::invalid_argument("scatter_strings: given strings for " + std::to_string(per_rank.size())
                                            + " ranks, but the communicator has " + std::to_string(mpi_num_procs));
            }
            send_sizes.resize(mpi_num_procs);
            displs.resize(mpi_num_procs);
            std::size_t total_size = 0;
            for (int r = 0; r < mpi_num_procs; ++r) {
                std::size_t rank_size = 0;
                for (const auto& s : per_rank[r]) {
                    rank_size += s.size() + 1;
                }
                if (rank_size > INT_MAX || total_size > INT_MAX - rank_size) {
                    throw std::length_error("scatter_strings: strings exceed INT_MAX bytes");
                }
                send_sizes[r] = static_cast<int>(rank_size);
                displs[r] = static_cast<int>(total_size);
                total_size += rank_size;
            }
            send_buffer.reserve(total_size);
            for (const auto& strings : per_rank) {
                for (const auto& s : strings) {
                    send_buffer.insert(send_buffer.end(), s.begin(), s.end());
                    send_buffer.push_back('\0');
                }
            }
        }

        int recv_size = 0;
        MPI_Scatter(send_sizes.data(), 1, MPI_INT, &recv_size, 1, MPI_INT, root, comm);

        std::vector<char> recv_buffer(recv_size);
        MPI_Scatterv(send_buffer.data(), send_sizes.data(), displs.data(), MPI_CHAR,
                     recv_buffer.data(), recv_size, MPI_CHAR, root, comm);

        std::vector<std::string> strings;
        auto string_start = recv_buffer.begin();
        for (auto it = recv_buffer.begin(); it != recv_buffer.end(); ++it) {
            if (*it == '\0') {
                strings.emplace_back(string_start, it);
                string_start = it + 1;
            }
        }
        return strings;
    }
} // namespace parallel

#endif // NGEN_WITH_MPI
