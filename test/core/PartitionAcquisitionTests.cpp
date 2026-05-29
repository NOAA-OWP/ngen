#include <gtest/gtest.h>

#include <PartitionAcquisition.hpp>

#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace {
const char* const PARTITION_JSON = R"({
  "partitions": [
    { "id": "0", "cat-ids": ["cat-0a", "cat-0b"], "nex-ids": ["nex-0"], "remote-connections": [] },
    { "id": "1", "cat-ids": ["cat-1"], "nex-ids": ["nex-1"], "remote-connections": [] }
  ]
})";

std::string write_partition_file()
{
    const auto path = std::filesystem::temp_directory_path() / "ngen_partition_acquisition_test.json";
    std::ofstream(path) << PARTITION_JSON;
    return path.string();
}

std::set<std::string> as_set(const std::vector<std::string>& values)
{
    return std::set<std::string>(values.begin(), values.end());
}
} // namespace

TEST(PartitionAcquisition_Test, LoadsRankZeroAndOverridesSubsets)
{
    const std::string path = write_partition_file();
    std::vector<std::string> catchment_subset_ids;
    std::vector<std::string> nexus_subset_ids;

    const PartitionData data = ngen::driver::load_partition_assignment(
        path, /*rank=*/0, catchment_subset_ids, nexus_subset_ids);

    EXPECT_EQ(data.mpi_world_rank, 0);
    EXPECT_EQ(data.catchment_ids, (std::unordered_set<std::string>{"cat-0a", "cat-0b"}));
    EXPECT_EQ(data.nexus_ids, (std::unordered_set<std::string>{"nex-0"}));
    EXPECT_EQ(as_set(catchment_subset_ids), (std::set<std::string>{"cat-0a", "cat-0b"}));
    EXPECT_EQ(as_set(nexus_subset_ids), (std::set<std::string>{"nex-0"}));

    std::filesystem::remove(path);
}

TEST(PartitionAcquisition_Test, SelectsPartitionByRank)
{
    const std::string path = write_partition_file();
    std::vector<std::string> catchment_subset_ids;
    std::vector<std::string> nexus_subset_ids;

    const PartitionData data = ngen::driver::load_partition_assignment(
        path, /*rank=*/1, catchment_subset_ids, nexus_subset_ids);

    EXPECT_EQ(data.mpi_world_rank, 1);
    EXPECT_EQ(as_set(catchment_subset_ids), (std::set<std::string>{"cat-1"}));
    EXPECT_EQ(as_set(nexus_subset_ids), (std::set<std::string>{"nex-1"}));

    std::filesystem::remove(path);
}
