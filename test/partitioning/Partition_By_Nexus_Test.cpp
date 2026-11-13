#include "gtest/gtest.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <FeatureBuilder.hpp>

#include "partitioning/Partition_By_Nexus.hpp"
#include "partitioning/Strategy_Nexus_Count.hpp"
#include "Partition_Test_Helpers.hpp"

using namespace ngen::partitioning;
using namespace partitioning_test;

namespace {

    struct CutCase
    {
        std::size_t num_items;
        int num_parts;
        std::vector<std::size_t> offsets;
    };


} // namespace

TEST(PartitionByNexusTest, CutEvenlyOffsets)
{
    std::vector<CutCase> const cases = {
        //  items  parts  offsets
          {  0,     1,    {0, 0}             }
        , {  0,     3,    {0, 0, 0, 0}       }
        , {  5,     1,    {0, 5}             }
        , {  8,     4,    {0, 2, 4, 6, 8}    }
        , { 10,     4,    {0, 3, 6, 8, 10}   }
        , { 11,     4,    {0, 3, 6, 9, 11}   }
        , {  2,     4,    {0, 1, 2, 2, 2}    }
        , {  7,     7,    {0, 1, 2, 3, 4, 5, 6, 7} }
    };
    for (const auto& c : cases) {
        EXPECT_EQ(cut_evenly(c.num_items, c.num_parts), c.offsets)
            << c.num_items << " items into " << c.num_parts << " parts";
    }
}

TEST(PartitionByNexusTest, CutEvenlyMinimizesLargestBlock)
{
    for (std::size_t items = 0; items <= 60; ++items) {
        for (int parts = 1; parts <= 13; ++parts) {
            auto const offsets = cut_evenly(items, parts);
            ASSERT_EQ(offsets.size(), static_cast<std::size_t>(parts) + 1);
            EXPECT_EQ(offsets.front(), 0u);
            EXPECT_EQ(offsets.back(), items);

            auto const sizes = block_sizes(offsets);
            std::size_t const ceiling = (items + parts - 1) / parts;
            EXPECT_EQ(*std::max_element(sizes.begin(), sizes.end()), ceiling)
                << items << " items into " << parts << " parts";
            EXPECT_EQ(*std::min_element(sizes.begin(), sizes.end()), items / parts)
                << items << " items into " << parts << " parts";
            EXPECT_TRUE(std::is_sorted(sizes.rbegin(), sizes.rend())) << "larger blocks come first";
        }
    }
}

TEST(PartitionByNexusTest, CutEvenlyRejectsNoParts)
{
    EXPECT_THROW(cut_evenly(5, 0), std::invalid_argument);
    EXPECT_THROW(cut_evenly(5, -1), std::invalid_argument);
}

TEST(PartitionByNexusTest, GroupByNexusKeepsFirstAppearanceOrder)
{
    NexusContributors const grouped = group_by_nexus(example_links());

    std::vector<std::optional<std::string>> const expected_nexuses = {"nex-1", "nex-2", "nex-3", "nex-4", "nex-5"};
    std::vector<std::vector<std::string>> const expected_catchments = {
          {"cat-11", "cat-12", "cat-13"}
        , {"cat-21"}
        , {"cat-31", "cat-32"}
        , {"cat-41"}
        , {"cat-51"}
    };
    EXPECT_EQ(grouped.nexus_ids, expected_nexuses);
    EXPECT_EQ(grouped.catchment_ids, expected_catchments);
}

TEST(PartitionByNexusTest, GroupByNexusOfNothing)
{
    NexusContributors const grouped = group_by_nexus({});
    EXPECT_TRUE(grouped.nexus_ids.empty());
    EXPECT_TRUE(grouped.catchment_ids.empty());
}

TEST(PartitionByNexusTest, PartitionKeepsCatchmentsWithTheirNexus)
{
    auto const links = example_links();
    for (int parts = 1; parts <= 7; ++parts) {
        SCOPED_TRACE("parts = " + std::to_string(parts));
        auto const partitions = partition_by_nexus(catchment_features(links), parts, Strategy::nexus_count);
        ASSERT_EQ(partitions.size(), static_cast<std::size_t>(parts));
        expect_consistent(links, partitions);
    }
}

TEST(PartitionByNexusTest, PartitionByNexusCount)
{
    auto const partitions = partition_by_nexus(catchment_features(example_links()), 2, Strategy::nexus_count);
    ASSERT_EQ(partitions.size(), 2u);

    EXPECT_EQ(partitions[0].nexus_ids, (std::unordered_set<std::string>{"nex-1", "nex-2", "nex-3"}));
    EXPECT_EQ(partitions[1].nexus_ids, (std::unordered_set<std::string>{"nex-4", "nex-5"}));
}

TEST(PartitionByNexusTest, PartitionRejectsBadInput)
{
    auto const catchments = catchment_features(example_links());
    EXPECT_THROW(partition_by_nexus(catchments, 0), std::invalid_argument);
    EXPECT_THROW(partition_by_nexus(catchments, 2, static_cast<Strategy>(-1)), std::invalid_argument);
}

TEST(PartitionByNexusTest, LinksFromCatchmentFeatures)
{
    std::stringstream data(R"({
        "type": "FeatureCollection",
        "features": [
            {"type": "Feature", "id": "cat-1", "properties": {"toid": "nex-1"}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}},
            {"type": "Feature", "id": "cat-2", "properties": {"toid": "nex-1"}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}},
            {"type": "Feature", "id": "cat-3", "properties": {"toid": "nex-2"}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}}
        ]
    })");
    auto const links = catchment_nexus_links(geojson::read(data));

    ASSERT_EQ(links.size(), 3u);
    EXPECT_EQ(links[0].catchment_id, "cat-1");
    EXPECT_EQ(links[0].nexus_id, "nex-1");
    EXPECT_EQ(links[1].catchment_id, "cat-2");
    EXPECT_EQ(links[1].nexus_id, "nex-1");
    EXPECT_EQ(links[2].catchment_id, "cat-3");
    EXPECT_EQ(links[2].nexus_id, "nex-2");
}

TEST(PartitionByNexusTest, PartitionCatchmentFeatures)
{
    std::stringstream data(R"({
        "type": "FeatureCollection",
        "features": [
            {"type": "Feature", "id": "cat-1", "properties": {"toid": "nex-1"}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}},
            {"type": "Feature", "id": "cat-2", "properties": {"toid": "nex-2"}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}},
            {"type": "Feature", "id": "cat-3", "properties": {"toid": "nex-1"}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}},
            {"type": "Feature", "id": "cat-4", "properties": {"toid": "nex-3"}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}}
        ]
    })");
    auto const partitions = partition_by_nexus(geojson::read(data), 2, Strategy::nexus_count);
    ASSERT_EQ(partitions.size(), 2u);

    EXPECT_EQ(partitions[0].nexus_ids, (std::unordered_set<std::string>{"nex-1", "nex-2"}));
    EXPECT_EQ(partitions[0].catchment_ids, (std::unordered_set<std::string>{"cat-1", "cat-2", "cat-3"}));
    EXPECT_EQ(partitions[1].nexus_ids, (std::unordered_set<std::string>{"nex-3"}));
    EXPECT_EQ(partitions[1].catchment_ids, (std::unordered_set<std::string>{"cat-4"}));
}

TEST(PartitionByNexusTest, LinksWithoutToidHaveNoNexus)
{
    std::stringstream data(R"({
        "type": "FeatureCollection",
        "features": [
            {"type": "Feature", "id": "cat-1", "properties": {"toid": "nex-1"}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}},
            {"type": "Feature", "id": "cat-2", "properties": {}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}}
        ]
    })");
    auto const links = catchment_nexus_links(geojson::read(data));

    ASSERT_EQ(links.size(), 2u);
    EXPECT_EQ(links[1].catchment_id, "cat-2");
    EXPECT_FALSE(links[1].nexus_id.has_value());
}

TEST(PartitionByNexusTest, CatchmentsWithoutNexusAreGroupsOfTheirOwn)
{
    std::vector<CatchmentNexusLink> const links = {
          {"cat-1", "nex-1"     }
        , {"cat-2", std::nullopt}
        , {"cat-3", "nex-1"     }
        , {"cat-4", std::nullopt}
    };
    NexusContributors const grouped = group_by_nexus(links);

    std::vector<std::optional<std::string>> const expected_nexuses = {"nex-1", std::nullopt, std::nullopt};
    std::vector<std::vector<std::string>> const expected_catchments = {
          {"cat-1", "cat-3"}
        , {"cat-2"}
        , {"cat-4"}
    };
    EXPECT_EQ(grouped.nexus_ids, expected_nexuses);
    EXPECT_EQ(grouped.catchment_ids, expected_catchments);

    auto const partitions = partition_by_nexus(catchment_features(links), 3);
    EXPECT_EQ(partitions[0].nexus_ids, (std::unordered_set<std::string>{"nex-1"}));
    EXPECT_EQ(partitions[0].catchment_ids, (std::unordered_set<std::string>{"cat-1", "cat-3"}));
    EXPECT_TRUE(partitions[1].nexus_ids.empty());
    EXPECT_EQ(partitions[1].catchment_ids, (std::unordered_set<std::string>{"cat-2"}));
    EXPECT_TRUE(partitions[2].nexus_ids.empty());
    EXPECT_EQ(partitions[2].catchment_ids, (std::unordered_set<std::string>{"cat-4"}));
}
