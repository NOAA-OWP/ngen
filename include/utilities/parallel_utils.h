#ifndef NGEN_PARALLEL_UTILS_H
#define NGEN_PARALLEL_UTILS_H

#include <NGenConfig.h>
#if NGEN_WITH_MPI

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
#include <vector>

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
    bool mpiSyncStatusAnd(bool status, int mpi_rank, int mpi_num_procs, const std::string &taskDesc);

    /**
     * Convenience method for overloaded function when no message is needed, and thus no description param provided.
     *
     * @param status The initial individual state for the current MPI rank.
     * @param mpi_rank The rank of the current process.
     * @param mpi_num_procs The total number of MPI processes.
     * @return Whether all ranks coordinating status had a success/ready status value.
     * @see mpiSyncStatusAnd(bool, const std::string&)
     */
    bool mpiSyncStatusAnd(bool status, int mpi_rank, int mpi_num_procs);

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
     * @param catchmentDataFile The path to the catchment data file for the hydrofabric.
     * @param mpi_rank The rank of the current process.
     * @param mpi_num_procs The total number of MPI processes.
     * @param printMessage Whether a supplemental message should be printed to standard out indicating status.
     *
     * @return Whether proprocessing has already been performed to divide the main hydrofabric into existing, individual
     *         sub-hydrofabric files for each partition/process.
     */
    bool is_hydrofabric_subdivided(const std::string &catchmentDataFile, int mpi_rank, int mpi_num_procs, bool printMsg);

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
    void get_hosts_array(int mpi_rank, int mpi_num_procs, int *host_array);

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
    bool mpi_send_text_file(const char *fileName, const int mpi_rank, const int destRank);

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
    bool mpi_recv_text_file(const char *fileName, const int mpi_rank, const int srcRank);

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
                                                 const int *hostIdForRank, bool syncReturnStatus, bool blockAll);

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
    bool subdivide_hydrofabric(int mpi_rank, int mpi_num_procs, const std::string &catchmentDataFile,
                               const std::string &nexusDataFile, const std::string &partitionConfigFile);

    /**
     * MPI_Gather vector<string> values from all processes. The result may include duplicate values.
     * 
     * @param local_strings Vector of strings from the current process
     * @param mpi_rank Rank of the current process
     * @param mpi_num_procs Number of processes
     * @return A vector of the gathered strings from all processes. This will only be populated if mpi_rank == 0
     */
    std::vector<std::string> gather_strings(const std::vector<std::string>& local_strings, int mpi_rank, int mpi_num_procs);

    /**
     * Send a vector<string> from root to all other processes.
     * 
     * @param strings If mpi_rank == 0, the strings that will be broadcasted. Unused for other processes.
     * @param mpi_rank Rank of the current process
     * @param mpi_num_procs Number of processes
     * @return vector<string> of the broadcasted strings from mpi_rank == 0
     */
    std::vector<std::string> broadcast_strings(const std::vector<std::string>& strings, int mpi_rank, int mpi_num_procs);
}
#endif // NGEN_WITH_MPI

#endif //NGEN_PARALLEL_UTILS_H
