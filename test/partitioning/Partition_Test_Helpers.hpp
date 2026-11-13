#ifndef NGEN_TEST_PARTITIONING_PARTITION_TEST_HELPERS_HPP
#define NGEN_TEST_PARTITIONING_PARTITION_TEST_HELPERS_HPP

#include "gtest/gtest.h"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/Partition_Data.hpp"
#include "partitioning/Partition_By_Nexus.hpp"

namespace partitioning_test {

    using namespace ngen::partitioning;
    using namespace ngen::partitioning::details;

    /// Catchment features with the IDs of @p links, each with its nexus, if any, under `toid`
    inline geojson::GeoJSON catchment_features(const std::vector<CatchmentNexusLink>& links)
    {
        std::stringstream data;
        data << R"({"type": "FeatureCollection", "features": [)";
        for (std::size_t i = 0; i < links.size(); ++i) {
            data << (i == 0 ? "" : ",") << R"({"type": "Feature", "id": ")" << links[i].catchment_id
                 << R"(", "properties": {)";
            if (links[i].nexus_id) {
                data << R"("toid": ")" << *links[i].nexus_id << '"';
            }
            data << R"(}, "geometry": {"type": "Point", "coordinates": [0.0, 0.0]}})";
        }
        data << "]}";
        return geojson::read(data);
    }

    inline std::vector<std::size_t> block_sizes(const std::vector<std::size_t>& offsets)
    {
        std::vector<std::size_t> sizes;
        for (std::size_t p = 0; p + 1 < offsets.size(); ++p) {
            sizes.push_back(offsets[p + 1] - offsets[p]);
        }
        return sizes;
    }

    /// A small network: nex-1 has three contributors, nex-2 one, nex-3 two, nex-4 one, nex-5 one
    inline std::vector<CatchmentNexusLink> example_links()
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
     * it drains to, to exactly one part.
     */
    inline void expect_consistent(const std::vector<CatchmentNexusLink>& links, const std::vector<PartitionData>& parts)
    {
        std::unordered_map<std::string, int> part_of_nexus;
        std::unordered_map<std::string, int> part_of_catchment;
        for (std::size_t p = 0; p < parts.size(); ++p) {
            EXPECT_EQ(parts[p].mpi_world_rank, static_cast<int>(p));
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

} // namespace partitioning_test

#endif // NGEN_TEST_PARTITIONING_PARTITION_TEST_HELPERS_HPP
