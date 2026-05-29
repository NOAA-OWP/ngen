#include <PartitionAcquisition.hpp>

#include <Partition_Parser.hpp>
#include <Partition_One.hpp>

#include <iostream>
#include <utility>
#include <vector>

namespace ngen {
namespace driver {

PartitionData load_partition_assignment(const std::string& partition_path,
                                        int rank,
                                        std::vector<std::string>& catchment_subset_ids,
                                        std::vector<std::string>& nexus_subset_ids)
{
    Partitions_Parser partition_parser(partition_path);
    // TODO: add something here to make sure this step worked for every rank, and maybe to checksum the file
    partition_parser.parse_partition_file();

    std::vector<PartitionData>& partitions = partition_parser.partition_ranks;
    PartitionData local_data = std::move(partitions[rank]);

    if (!nexus_subset_ids.empty()) {
        std::cerr << "Warning: CLI provided nexus subset will be ignored when using partition config";
    }
    if (!catchment_subset_ids.empty()) {
        std::cerr << "Warning: CLI provided catchment subset will be ignored when using partition config";
    }
    nexus_subset_ids = std::vector<std::string>(local_data.nexus_ids.begin(), local_data.nexus_ids.end());
    catchment_subset_ids = std::vector<std::string>(local_data.catchment_ids.begin(), local_data.catchment_ids.end());

    return local_data;
}

PartitionData single_process_partition(geojson::GeoJSON& catchment_collection)
{
    Partition_One partition_one;
    partition_one.generate_partition(catchment_collection);
    return std::move(partition_one.partition_data);
}

} // namespace driver
} // namespace ngen
