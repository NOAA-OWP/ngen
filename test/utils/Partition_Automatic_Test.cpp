#include "gtest/gtest.h"

#include <NGenConfig.h>

#if NGEN_WITH_MPI

#include <algorithm>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <mpi.h>

#include "utilities/Partition_By_Nexus.hpp"
#include "utilities/Partition_Automatic.hpp"
#include "utilities/parallel_utils.h"
#include "utilities/FileChecker.h"

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
    EXPECT_FALSE(local.nexus_ids.empty());
}

TEST(AutomaticPartitionTest, ScatterPartitionsRejectsWrongPartCount)
{
    std::vector<PartitionData> const partitions(2);
    EXPECT_THROW(scatter_partitions(partitions, 0, MPI_COMM_SELF), std::invalid_argument);
}

#ifdef NGEN_WITH_SQLITE3

namespace {

    std::string gauge_hydrofabric()
    {
        return utils::FileChecker::find_first_readable({
            "data/gauge_01073000/gauge_01073000.gpkg",
            "../data/gauge_01073000/gauge_01073000.gpkg",
            "../../data/gauge_01073000/gauge_01073000.gpkg"
        });
    }

    /// Every rank's catchments, gathered to rank 0 of @p comm
    std::vector<std::string> all_catchments(const PartitionData& local, MPI_Comm comm)
    {
        std::vector<std::string> mine(local.catchment_ids.begin(), local.catchment_ids.end());
        std::vector<std::string> all = parallel::gather_strings(mine, comm);
        std::sort(all.begin(), all.end());
        return all;
    }

} // namespace

// The example hydrofabric has five divides draining to three nexuses:
// cat-11223, cat-11371 and cat-11509 to nex-11224, cat-11410 to nex-11223, and cat-11224 to nex-11225
TEST(AutomaticPartitionTest, PartitionAutomaticallyFromGeoPackage)
{
    std::string const path = gauge_hydrofabric();
    ASSERT_FALSE(path.empty()) << "can't find data/gauge_01073000/gauge_01073000.gpkg";
    int const size = comm_size(MPI_COMM_WORLD);
    int const root = size - 1;

    auto const local = partition_automatically(path, path, {}, root, MPI_COMM_WORLD);
    if (size > 3) {
        EXPECT_FALSE(local.has_value()) << "more ranks than nexuses leaves a rank empty";
        return;
    }
    ASSERT_TRUE(local.has_value());
    EXPECT_FALSE(local->catchment_ids.empty());

    std::set<std::string> const together = {"cat-11223", "cat-11371", "cat-11509"};
    auto const held = std::count_if(together.begin(), together.end(),
                                    [&](const std::string& id) { return local->catchment_ids.count(id) > 0; });
    EXPECT_TRUE(held == 0 || held == 3) << "catchments draining to nex-11224 are split across ranks";
    EXPECT_EQ(local->nexus_ids.count("nex-11224") > 0, held == 3);

    std::vector<std::string> const all = all_catchments(*local, MPI_COMM_WORLD);
    if (comm_rank(MPI_COMM_WORLD) == 0) {
        std::vector<std::string> const expected = {"cat-11223", "cat-11224", "cat-11371", "cat-11410", "cat-11509"};
        EXPECT_EQ(all, expected);
    }
}

TEST(AutomaticPartitionTest, PartitionAutomaticallyASubset)
{
    std::string const path = gauge_hydrofabric();
    ASSERT_FALSE(path.empty()) << "can't find data/gauge_01073000/gauge_01073000.gpkg";
    if (comm_size(MPI_COMM_WORLD) > 2) {
        GTEST_SKIP() << "the subset drains to only two nexuses";
    }

    auto const local = partition_automatically(path, path, {"cat-11410", "cat-11224"}, 0, MPI_COMM_WORLD);
    ASSERT_TRUE(local.has_value());

    std::vector<std::string> const all = all_catchments(*local, MPI_COMM_WORLD);
    if (comm_rank(MPI_COMM_WORLD) == 0) {
        std::vector<std::string> const expected = {"cat-11224", "cat-11410"};
        EXPECT_EQ(all, expected);
    }
}

TEST(AutomaticPartitionTest, PartitionAutomaticallyUnreadable)
{
    EXPECT_FALSE(partition_automatically("no-such-hydrofabric.gpkg", "no-such-hydrofabric.gpkg", {}, 0, MPI_COMM_WORLD)
                     .has_value());
}

#endif // NGEN_WITH_SQLITE3

#endif // NGEN_WITH_MPI
