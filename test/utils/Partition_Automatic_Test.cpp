#include "gtest/gtest.h"

#include <NGenConfig.h>

#if NGEN_WITH_MPI

#include <stdexcept>
#include <string>
#include <vector>

#include <mpi.h>

#include "utilities/Partition_By_Nexus.hpp"
#include "utilities/Partition_Automatic.hpp"
#include "utilities/parallel_utils.h"

using namespace ngen::partitioning;

namespace {

    int comm_rank(MPI_Comm comm)
    {
        int rank;
        MPI_Comm_rank(comm, &rank);
        return rank;
    }

    int comm_size(MPI_Comm comm)
    {
        int size;
        MPI_Comm_size(comm, &size);
        return size;
    }

    /// Rank r receives r + 1 strings, the second of which is empty
    std::vector<std::string> strings_for_rank(int r)
    {
        std::vector<std::string> strings;
        for (int i = 0; i <= r; ++i) {
            strings.push_back(i == 1 ? "" : "rank-" + std::to_string(r) + "-item-" + std::to_string(i));
        }
        return strings;
    }

    /// More nexuses than ranks, with varying numbers of contributing catchments
    std::vector<CatchmentNexusLink> links_for(int num_ranks)
    {
        std::vector<CatchmentNexusLink> links;
        int const num_nexuses = 3 * num_ranks + 2;
        for (int n = 0; n < num_nexuses; ++n) {
            for (int c = 0; c <= n % 4; ++c) {
                links.push_back({"cat-" + std::to_string(n) + "-" + std::to_string(c), "nex-" + std::to_string(n)});
            }
        }
        return links;
    }

} // namespace

TEST(AutomaticPartitionTest, ScatterStrings)
{
    int const root = comm_size(MPI_COMM_WORLD) - 1;
    std::vector<std::vector<std::string>> per_rank;
    if (comm_rank(MPI_COMM_WORLD) == root) {
        for (int r = 0; r < comm_size(MPI_COMM_WORLD); ++r) {
            per_rank.push_back(strings_for_rank(r));
        }
    }
    EXPECT_EQ(parallel::scatter_strings(per_rank, root, MPI_COMM_WORLD), strings_for_rank(comm_rank(MPI_COMM_WORLD)));
}

TEST(AutomaticPartitionTest, ScatterStringsRejectsWrongRankCount)
{
    std::vector<std::vector<std::string>> const per_rank(2);
    EXPECT_THROW(parallel::scatter_strings(per_rank, 0, MPI_COMM_SELF), std::invalid_argument);
}

TEST(AutomaticPartitionTest, ScatterPartitions)
{
    int const rank = comm_rank(MPI_COMM_WORLD);
    int const size = comm_size(MPI_COMM_WORLD);
    int const root = size - 1;
    auto const all = partition_by_nexus(group_by_nexus(links_for(size)), size);

    PartitionData const local = scatter_partitions(rank == root ? all : std::vector<PartitionData>{}, root,
                                                   MPI_COMM_WORLD);
    PartitionData const& expected = all[rank];

    EXPECT_EQ(local.mpi_world_rank, rank);
    EXPECT_EQ(local.nexus_ids, expected.nexus_ids);
    EXPECT_EQ(local.catchment_ids, expected.catchment_ids);
    EXPECT_TRUE(local.remote_connections.empty());
    EXPECT_FALSE(local.nexus_ids.empty());
}

TEST(AutomaticPartitionTest, ScatterPartitionsRejectsWrongPartCount)
{
    std::vector<PartitionData> const partitions(2);
    EXPECT_THROW(scatter_partitions(partitions, 0, MPI_COMM_SELF), std::invalid_argument);
}

TEST(AutomaticPartitionTest, ScatterPartitionsRejectsRemoteConnections)
{
    std::vector<PartitionData> partitions(1);
    partitions[0].remote_connections.emplace_back(1, "nex-1", "cat-1", "orig_cat-to-nex");
    EXPECT_THROW(scatter_partitions(partitions, 0, MPI_COMM_SELF), std::invalid_argument);
}

#endif // NGEN_WITH_MPI
