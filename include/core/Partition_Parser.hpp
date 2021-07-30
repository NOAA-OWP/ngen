#ifndef PARTITION_PARSER_H
#define PARTITION_PARSER_H

#ifdef NGEN_MPI_ACTIVE

//#include <mpi.h>

#include <memory>
#include <sstream>
#include <tuple>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <FeatureBuilder.hpp>
#include "features/Features.hpp"
#include <FeatureCollection.hpp>
#include "JSONProperty.hpp"

//This struct is moved from private section to here so that the unit test function can access it
 struct PartitionData
 {
    int mpi_world_rank;
    std::vector<std::string> cat_ids;
    std::vector<std::string> nex_ids;
    std::vector<std::string> remote_nexi;
    std::vector<std::string> remote_catchments;
    std::vector<int> remote_mpi_ranks;
 };


class Partitions_Parser {

    public:
        //Constructor that takes an input json file path and points to the root of the tree in json file
        Partitions_Parser(const std::string &file_path) {
            boost::property_tree::ptree loaded_tree;
            boost::property_tree::json_parser::read_json(file_path, loaded_tree);
            this->tree = loaded_tree;
            std::cout << "file_path: " << file_path << std::endl;
        };

        virtual ~Partitions_Parser(){};

        //The function that parses the json file and build a unordered map of structs for each line in the json list
        void parse_partition_file() {
            std::cout << "\nroot_tree: " << tree.size() << std::endl;

            //Declare two partition_data type structs
            PartitionData part_data;

            //Declare temporary variables
            std::vector<std::string> remote_nexi_tmp;
            std::vector<std::string> remote_catchments_tmp;
            std::vector<int> remote_mpi_ranks_tmp;

            //The outter loop iterating through the list of partitions
            int part_counter = 0;
            for(auto &partition: tree.get_child("partitions"))  {
                std::string part_id = (partition.second).get<std::string>("id");

                //Get mpi_world_rank and set the corresponding part_data struct member
                int mpi_rank = std::stoi(part_id);
                part_data.mpi_world_rank = mpi_rank;

                geojson::JSONProperty part = geojson::JSONProperty(part_id, partition.second);

                //Get cat_ids list and set the corresponding part_data struct member
                for (auto &remote : part.at("cat-ids").as_list())
                {
                    cat_ids.push_back(remote.as_string());
                }
                part_data.cat_ids = cat_ids;
                cat_ids.clear();

                //Get nex_ids list and set the corresponding part_data struct member
                for (auto &remote : part.at("nex-ids").as_list())
                {
                    nex_ids.push_back(remote.as_string());
                }
                part_data.nex_ids = nex_ids;
                nex_ids.clear();

                //Get remote-connections and set the corresponding part_data struct member
                for (auto &remote_map : part.at("remote-connections").as_list())
                {
                    std::string remote_nex_id = remote_map.at("nex-id").as_string();
                    std::string remote_cat_id = remote_map.at("cat-id").as_string();
                    int remote_mpi_rank = remote_map.at("mpi-rank").as_natural_number();
                    remote_nexi_tmp.push_back(remote_nex_id);
                    remote_catchments_tmp.push_back(remote_cat_id);
                    remote_mpi_ranks_tmp.push_back(remote_mpi_rank);
                }
                part_data.remote_nexi = remote_nexi_tmp;
                part_data.remote_catchments = remote_catchments_tmp;
                part_data.remote_mpi_ranks = remote_mpi_ranks_tmp;
                remote_nexi_tmp.clear();
                remote_catchments_tmp.clear();
                remote_mpi_ranks_tmp.clear();

                //Insert part_data struct into unordered map based on part_id
                partition_ranks.emplace(part_id, part_data);

                part_counter++;       
            }
            
            //return;
        };

        typedef PartitionData part_strt;
        //The function retrieve an arbitrary struct based on the part_id passed to it
        part_strt get_part_strt(std::string part_id)
        {
            //TODO The output codes are for test only. They can be removed for application code.
            /*
             * The function takes a partition id and return the corresponding struct from the map.
             * @param part_id The input parameter identify a specific partition struct
             * @return part_data The whole struct identified by part_id
            */
            PartitionData part_data;
            part_data = partition_ranks.at(part_id);

            std::cout << "\nget_part_strt, part_id: " << part_data.mpi_world_rank << std::endl;

            for (auto i = (part_data.cat_ids).begin(); i != (part_data.cat_ids).end(); ++i)
                std::cout << "\nget_part_strt, cat_ids: " << *i << " ";
            std::cout << std::endl;

            for (auto i = part_data.remote_nexi.begin(); i != part_data.remote_nexi.end(); ++i)
                std::cout << "\nget_part_strt, remote_nexi: " << *i << " ";
            std::cout << std::endl;

            for (auto i = part_data.remote_catchments.begin(); i != part_data.remote_catchments.end(); ++i)
                std::cout << "\nget_part_strt, remote_catchments: " << *i << " ";
            std::cout << std::endl;

            for (auto i = part_data.remote_mpi_ranks.begin(); i != part_data.remote_mpi_ranks.end(); ++i)
                std::cout << "\nget_part_strt, remote_mpi_ranks: " << *i << " ";
            std::cout << std::endl;

            std::cout << "--------------------" << std::endl;

            return part_data;
        };

        //This example function shows how to get a specific member of the struct
        int get_mpi_rank(std::string part_id)
        {
            //An example code for getting individual member of the struct identified by part_id
            PartitionData part_data;
            part_data = partition_ranks.at(part_id);
            int mpi_world_rank = part_data.mpi_world_rank;

            std::cout << "\nmpi_world_rank: " << mpi_world_rank << std::endl;
            return mpi_world_rank;
        };

        std::unordered_map<std::string, PartitionData> partition_ranks;


    private:
        int mpi_world_rank;
        std::vector<std::string> cat_ids;
        std::vector<std::string> nex_ids;
        std::vector<std::string> remote_nexi;
        std::vector<std::string> remote_catchments;
        std::vector<int> remote_mpi_ranks;

        boost::property_tree::ptree tree;
};

#endif // NGEN_MPI_ACTIVE
#endif // PARTITION_PARSER_H
