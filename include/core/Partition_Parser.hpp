#ifndef PARTITION_PARSER_H
#define PARTITION_PARSER_H

#include <memory>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>

#include <boost/property_tree/ptree.hpp>
#include <FeatureBuilder.hpp>
#include "features/Features.hpp"
#include <FeatureCollection.hpp>
#include "JSONProperty.hpp"
#include "Partition_Data.hpp"

class Partitions_Parser {

    public:
        Partitions_Parser(const boost::property_tree::ptree tree){
            this->tree = tree;
        }

        virtual ~Partitions_Parser(){};

        //The function that parses the json file and build a unordered set and vector of structs for each line in the json list
        void parse_partition_file() {
            std::cout << "\nroot_tree: " << tree.size() << std::endl;

            //Declare two partition_data type structs
            PartitionData part_data;

            //The outter loop iterating through the list of partitions
            int part_counter = 0;
            for(auto &partition: tree.get_child("partitions"))  {
                //Get partition id
                std::string part_id = (partition.second).get<std::string>("id");

                //Get mpi_world_rank and set the corresponding part_data struct member
                int mpi_rank = std::stoi(part_id);
                part_data.mpi_world_rank = mpi_rank;

                geojson::JSONProperty part = geojson::JSONProperty(part_id, partition.second);

                //Get cat_ids list and insert the elements into the unordered_set catchment_ids in part_data struct
                for (auto &cat_id : part.at("cat-ids").as_list())
                {
                    //cat_ids.push_back(remote.as_string());
                    catchment_ids.emplace(cat_id.as_string());
                }
                part_data.catchment_ids = catchment_ids;
                catchment_ids.clear();

                //Get nex_ids list and insert the elements into the unordered_set nexus_ids in part_data struct
                for (auto &nex_id: part.at("nex-ids").as_list())
                {
                    //nex_ids.push_back(nex_id.as_string());
                    nexus_ids.emplace(nex_id.as_string());
                }
                part_data.nexus_ids = nexus_ids;
                nexus_ids.clear();

                //Remote connections between partitions are not supported: a partition must hold every
                //catchment that drains to each of its nexuses. The backend property tree parser can't
                //represent an empty list, so an empty "remote-connections" arrives as an empty string.
                if (part.has_key("remote-connections")
                    && part.at("remote-connections").get_type() == geojson::PropertyType::List
                    && !part.at("remote-connections").as_list().empty())
                {
                    throw std::runtime_error("Partition " + part_id + " has remote connections, which are not "
                                             "supported: each partition must hold every catchment that drains "
                                             "to each of its nexuses");
                }

                //Push part_data struct the vector
                partition_ranks.push_back(part_data);

                part_counter++;   
                
            }
        };

        //The function retrieve an arbitrary struct based on the part_id passed to it
        PartitionData get_partition_struct(int part_id)
        {
            //TODO The output codes are for test only. They can be removed for application code.
            /*
             * The function takes a partition id and return the corresponding struct.
             * @param part_id The input parameter identify a specific partition struct
             * @return part_data The whole struct identified by part_id
            */
            PartitionData part_data;
            part_data = partition_ranks[part_id];

            // the following blocks of codes write out the data for validation
            /*
            std::cout << "\nget_partition_struct, part_id: " << part_data.mpi_world_rank << std::endl;

            for (auto i = (part_data.catchment_ids).cbegin(); i != (part_data.catchment_ids).cend(); ++i)
                std::cout << "\nget_partition_struct, catchment_ids: " << *i << " ";
            std::cout << std::endl;

            for (auto i = (part_data.nexus_ids).cbegin(); i != (part_data.nexus_ids).cend(); ++i)
                std::cout << "\nget_partition_struct, nexus_ids: " << *i << " ";
            std::cout << std::endl;
            std::cout << "\n--------------------" << std::endl;
            */

            return part_data;
        }

        //This example function shows how to get a specific member of the struct
        int get_mpi_rank(int part_id)
        {
            //An example code for getting individual member of the struct identified by part_id
            PartitionData part_data;
            part_data = partition_ranks[part_id];
            int mpi_world_rank = part_data.mpi_world_rank;

            std::cout << "mpi_world_rank: " << mpi_world_rank << std::endl;
            return mpi_world_rank;
        }

        // partition_ranks is a vector of struct: PartitionData
        std::vector<PartitionData> partition_ranks;

    private:
        int mpi_world_rank;
        std::unordered_set<std::string> catchment_ids;
        std::unordered_set<std::string> nexus_ids;

        boost::property_tree::ptree tree;
};

#endif // PARTITION_PARSER_H
