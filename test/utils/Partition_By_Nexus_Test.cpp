#include "gtest/gtest.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <FeatureBuilder.hpp>

#include "utilities/Partition_By_Nexus.hpp"

using namespace ngen::partitioning;

namespace {

    struct CutCase
    {
        std::size_t num_items;
        int num_parts;
        std::vector<std::size_t> offsets;
    };

    std::vector<std::size_t> block_sizes(const std::vector<std::size_t>& offsets)
    {
        std::vector<std::size_t> sizes;
        for (std::size_t p = 0; p + 1 < offsets.size(); ++p) {
            sizes.push_back(offsets[p + 1] - offsets[p]);
        }
        return sizes;
    }

    /// A small network: nex-1 has three contributors, nex-2 one, nex-3 two, nex-4 one, nex-5 one
    std::vector<CatchmentNexusLink> example_links()
    {
        return {
              {"cat-11", "nex-1"}
            , {"cat-21", "nex-2"}
            , {"cat-12", "nex-1"}
            , {"cat-31", "nex-3"}
            , {"cat-13", "nex-1"}
            , {"cat-41", "nex-4"}
            , {"cat-32", "nex-3"}
            , {"cat-51", "nex-5"}
        };
    }

    /**
     * Check that @p parts assigns each nexus of @p links, and each catchment along with the nexus
     * it drains to, to exactly one part, with no remote connections.
     */
    void expect_consistent(const std::vector<CatchmentNexusLink>& links, const std::vector<PartitionData>& parts)
    {
        std::unordered_map<std::string, int> part_of_nexus;
        std::unordered_map<std::string, int> part_of_catchment;
        for (std::size_t p = 0; p < parts.size(); ++p) {
            EXPECT_EQ(parts[p].mpi_world_rank, static_cast<int>(p));
            EXPECT_TRUE(parts[p].remote_connections.empty());
            for (const auto& id : parts[p].nexus_ids) {
                EXPECT_TRUE(part_of_nexus.emplace(id, p).second) << "nexus " << id << " in more than one part";
            }
            for (const auto& id : parts[p].catchment_ids) {
                EXPECT_TRUE(part_of_catchment.emplace(id, p).second) << "catchment " << id << " in more than one part";
            }
        }
        std::size_t num_catchments = 0;
        for (const auto& link : links) {
            ++num_catchments;
            ASSERT_EQ(part_of_catchment.count(link.catchment_id), 1) << "catchment " << link.catchment_id << " unassigned";
            if (!link.nexus_id) {
                continue;
            }
            ASSERT_EQ(part_of_nexus.count(*link.nexus_id), 1) << "nexus " << *link.nexus_id << " unassigned";
            EXPECT_EQ(part_of_catchment[link.catchment_id], part_of_nexus[*link.nexus_id])
                << "catchment " << link.catchment_id << " is apart from its nexus " << *link.nexus_id;
        }
        EXPECT_EQ(part_of_catchment.size(), num_catchments);
        auto const nexus_ids = group_by_nexus(links).nexus_ids;
        EXPECT_EQ(part_of_nexus.size(), static_cast<std::size_t>(std::count_if(nexus_ids.begin(), nexus_ids.end(),
                                                                            [](auto const& id) { return id.has_value(); })));
    }

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
        auto const partitions = partition_by_nexus(group_by_nexus(links), parts);
        ASSERT_EQ(partitions.size(), static_cast<std::size_t>(parts));
        expect_consistent(links, partitions);
    }
}

TEST(PartitionByNexusTest, PartitionCutsNexusesEvenly)
{
    auto const partitions = partition_by_nexus(group_by_nexus(example_links()), 2);
    ASSERT_EQ(partitions.size(), 2u);

    EXPECT_EQ(partitions[0].nexus_ids, (std::unordered_set<std::string>{"nex-1", "nex-2", "nex-3"}));
    EXPECT_EQ(partitions[0].catchment_ids,
              (std::unordered_set<std::string>{"cat-11", "cat-12", "cat-13", "cat-21", "cat-31", "cat-32"}));
    EXPECT_EQ(partitions[1].nexus_ids, (std::unordered_set<std::string>{"nex-4", "nex-5"}));
    EXPECT_EQ(partitions[1].catchment_ids, (std::unordered_set<std::string>{"cat-41", "cat-51"}));
}

TEST(PartitionByNexusTest, PartitionRejectsNoParts)
{
    EXPECT_THROW(partition_by_nexus(group_by_nexus(example_links()), 0), std::invalid_argument);
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

    auto const partitions = partition_by_nexus(grouped, 3);
    EXPECT_EQ(partitions[0].nexus_ids, (std::unordered_set<std::string>{"nex-1"}));
    EXPECT_EQ(partitions[0].catchment_ids, (std::unordered_set<std::string>{"cat-1", "cat-3"}));
    EXPECT_TRUE(partitions[1].nexus_ids.empty());
    EXPECT_EQ(partitions[1].catchment_ids, (std::unordered_set<std::string>{"cat-2"}));
    EXPECT_TRUE(partitions[2].nexus_ids.empty());
    EXPECT_EQ(partitions[2].catchment_ids, (std::unordered_set<std::string>{"cat-4"}));
}
