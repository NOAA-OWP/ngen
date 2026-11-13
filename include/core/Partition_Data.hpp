#ifndef PARTITION_DATA_H
#define PARTITION_DATA_H

#include <string>
#include <unordered_set>

struct PartitionData
{
    int mpi_world_rank;
    std::unordered_set<std::string> catchment_ids;
    std::unordered_set<std::string> nexus_ids;
};

#endif  //PARTITION_DATA_H
