#ifndef PARTITION_ONE_H
#define PARTITION_ONE_H

#ifdef NGEN_MPI_ACTIVE

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <FeatureBuilder.hpp>
#include "features/Features.hpp"
#include <FeatureCollection.hpp>
#include "Partition_Data.hpp"

class Partition_One {

    public:
        Partition_One() {};

        void generate_partition(geojson::GeoJSON& catchment_collection)
        {
            int counter = 0;
            for(auto& feature: *catchment_collection)
            {
                std::string cat_id = feature->get_id();
                catchment_ids.emplace(cat_id);
                std::string nex_id = feature->get_property("toid").as_string();
                nexus_ids.emplace(nex_id);
                counter++;
            }
            std::cout << "counter = " << counter << std::endl;
        }

    	virtual ~Partition_One(){};

        PartitionData partition_data;       

    private:
        int mpi_world_rank;
        std::unordered_set<std::string> catchment_ids;
        std::unordered_set<std::string> nexus_ids;
        std::vector<Tuple> remote_connections;
};

#endif // NGEN_MPI_ACTIVE
#endif // PARTITION_ONE_H
