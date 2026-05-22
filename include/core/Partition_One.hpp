#ifndef PARTITION_ONE_H
#define PARTITION_ONE_H

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
        /**
         * The function that parses geojson::GeoJSON data and build unordered sets of catchment_ids and nexus_ids

         * @param catchment_collection the geojson::GeoJSON data containing all the necessary hydrofabric info
         */
        void generate_partition(geojson::GeoJSON& catchment_collection)
        {
            for(auto& feature: *catchment_collection)
            {
                std::string cat_id = feature->get_id();
                partition_data.catchment_ids.emplace(cat_id);
                std::string nex_id = feature->get_property("toid").as_string();
                partition_data.nexus_ids.emplace(nex_id);
            }
            partition_data.mpi_world_rank = 0;
        }

        PartitionData partition_data;       
};

#endif // PARTITION_ONE_H
