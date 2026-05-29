#ifndef NGEN_DRIVER_PARTITION_ACQUISITION_HPP
#define NGEN_DRIVER_PARTITION_ACQUISITION_HPP

#include <string>
#include <vector>

#include <Partition_Data.hpp>
#include <FeatureBuilder.hpp> // for geojson::GeoJSON

namespace ngen {
namespace driver {

/**
 * Parse the partition config and return the partition data for the given rank.
 *
 * The catchment and nexus subset id vectors are replaced with the ids the
 * partition config assigns to that rank; if either is non-empty on entry, a
 * warning is emitted that the CLI-provided subset is ignored in favor of the
 * partition config. Intended for multi-process runs.
 *
 * This is the point at which the partition config governs the subset, so it
 * must run before the hydrofabric collections are read.
 *
 * @param rank The rank (and index into the config's partition list) to load.
 */
PartitionData load_partition_assignment(const std::string& partition_path,
                                        int rank,
                                        std::vector<std::string>& catchment_subset_ids,
                                        std::vector<std::string>& nexus_subset_ids);

/**
 * Build a partition spanning the entire catchment collection, for a
 * single-process run. Requires the catchment collection, so it runs after the
 * hydrofabric has been read.
 */
PartitionData single_process_partition(geojson::GeoJSON& catchment_collection);

} // namespace driver
} // namespace ngen

#endif // NGEN_DRIVER_PARTITION_ACQUISITION_HPP
