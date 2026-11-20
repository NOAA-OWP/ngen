#ifndef NGEN_PARALLEL_UTILS_H
#define NGEN_PARALLEL_UTILS_H

#include <NGenConfig.h>
#if NGEN_WITH_MPI

#include <mpi.h>
#include <string>
#include <vector>

namespace parallel {

    /**
     * MPI_Gather vector<string> values from all processes. The result may include duplicate values.
     * 
     * @param local_strings Vector of strings from the current process
     * @param comm The MPI communicator over which to gather
     * @return A vector of the gathered strings from all processes. This will only be populated on the root rank (rank 0)
     */
    std::vector<std::string> gather_strings(const std::vector<std::string>& local_strings, MPI_Comm comm);

    /**
     * Send a vector<string> from root to all other processes.
     * 
     * @param strings If on the root rank (rank 0), the strings that will be broadcasted. Unused for other processes.
     * @param comm The MPI communicator over which to broadcast
     * @return vector<string> of the broadcasted strings from the root rank (rank 0)
     */
    std::vector<std::string> broadcast_strings(const std::vector<std::string>& strings, MPI_Comm comm);

    /**
     * MPI_Scatterv a distinct vector<string> from @p root to each process.
     *
     * Strings must not contain NUL characters, and the strings for any one process, each with a
     * terminating NUL, must total fewer than INT_MAX bytes.
     *
     * @param per_rank On @p root, the strings for each rank of @p comm, indexed by rank. Unused on other processes.
     * @param root The rank sending the strings
     * @param comm The MPI communicator over which to scatter
     * @return The strings @p root provided for this process
     * @throws std::invalid_argument on @p root, before any communication, if @p per_rank does not have
     *         one entry per rank of @p comm
     * @throws std::length_error on @p root, before any communication, if any rank's strings exceed INT_MAX bytes
     */
    std::vector<std::string> scatter_strings(const std::vector<std::vector<std::string>>& per_rank, int root, MPI_Comm comm);
}

#endif // NGEN_WITH_MPI

#endif //NGEN_PARALLEL_UTILS_H
