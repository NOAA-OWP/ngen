#ifndef PARTITION_DATA_H
#define PARTITION_DATA_H

struct PartitionData
{
    using Tuple = std::tuple<int, std::string, std::string, std::string>;
    int mpi_world_rank;
    std::unordered_set<std::string> catchment_ids;
    std::unordered_set<std::string> nexus_ids;
    std::vector<Tuple> remote_connections;
};

#endif  //PARTITION_DATA_H
