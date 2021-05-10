#ifndef Partion_H
#define Partion_H

//#ifdef NGEN_MPI_ACTIVE

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

#define PARTITION_LINES 2

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

        //The function that parses the json file and build a unordered map of struct for each line in the json list
        //void read_partition_file(boost::property_tree::ptree &config);
        void read_partition_file(boost::property_tree::ptree &config) {
            tree = config;
            std::cout << "\nroot_tree: " << tree.size() << std::endl;

            //Declare two partition_data type structs
            partition_data part_data;
            partition_data part_strt;

            //The outter loop iterating through the list of partitions
            int part_counter = 0;
            for(auto &partition: tree.get_child("partitions"))  {
                std::string part_id = (partition.second).get<std::string>("id");

                //Get mpi_world_rank and set the corresponding part_data struct member
                mpi_world_rank = std::stoi(part_id);
                part_data.mpi_world_rank = mpi_world_rank;

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

                //Get remote-up and set the corresponding part_data struct member
                //For emplty list, remote_up_nex is set to empty string and remote_up_rank is set to -1
                //This distinguishes them from normal values and make it easy to handle in applications
                if (part.at("remote-up").as_list().empty() )
                {
                    remote_up_nex = "";
                    remote_up_rank = -1;
                } else {
                    for (auto &remote : part.at("remote-up").as_list())
                    {
                        remote_up_nex = remote.at("id").as_string();
                        remote_up_rank = remote.at("partition").as_natural_number();
                    }
                }
                part_data.remote_up_nex = remote_up_nex;
                part_data.remote_up_rank = remote_up_rank;

                //Get remote-down and set the corresponding part_data struct member
                //For emplty list, it is handled the same way as remote-up
                if (part.at("remote-down").as_list().empty() )
                {
                    remote_down_nex = "";
                    remote_down_rank = -1;
                } else {
                    for (auto &remote : part.at("remote-down").as_list())
                    {
                        remote_down_nex = remote.at("id").as_string();
                        remote_down_rank = remote.at("partition").as_natural_number();
                    }
                }
                part_data.remote_down_nex = remote_down_nex;
                part_data.remote_down_rank = remote_down_rank;

                //Insert part_data struct into unordered map based on part_id
                partition_ranks.emplace(part_id, part_data);

                part_counter++;       
            }

            if (part_counter != PARTITION_LINES)
            {
                std::string message = "Number of lines read does not match the number of lines in the input partition";
                throw std::runtime_error(message);
            }
        };

        //This struct is moved from private section to here so that the unit test function can access it
        struct partition_data
        {
            int mpi_world_rank;
            std::vector<std::string> cat_ids;
            std::vector<std::string> nex_ids;
            std::string remote_up_nex;
            int remote_up_rank;
            std::string remote_down_nex;
            int remote_down_rank;
        };

        typedef partition_data part_strt;
        //The function retrieve any arbitrary struct based on the part_id handed to it
        //part_strt get_part_strt(std::string part_id);
        part_strt get_part_strt(std::string part_id)
        {
            //TODO The output codes are for test only. They can be removed for application code.
            /*
             * The function takes a partition id and return the corresponding struct from the map.
             * @param part_id The input parameter identify a specific partition struct
             * @return part_data The whole struct identified by part_id
            */
            partition_data part_data;
            part_data = partition_ranks.at(part_id);

            std::cout << "\nget_part_strt, part_id: " << part_data.mpi_world_rank << std::endl;

            for (auto i = (part_data.cat_ids).begin(); i != (part_data.cat_ids).end(); ++i)
                std::cout << "\nget_part_strt, cat_ids: " << *i << " ";
            std::cout << std::endl;

            for (auto i = part_data.nex_ids.begin(); i != part_data.nex_ids.end(); ++i)
                std::cout << "\nget_part_strt, nex_ids: " << *i << " ";
            std::cout << std::endl;

            remote_up_nex = part_data.remote_up_nex;
            remote_up_rank = part_data.remote_up_rank;
            std::cout << "\nremote_up_nex: " << remote_up_nex << std::endl;
            std::cout << "remote_up_rank: " << remote_up_rank << std::endl;

            remote_down_nex = part_data.remote_down_nex;
            remote_down_rank = part_data.remote_down_rank;
            std::cout << "\nremote_down_nex: " << remote_down_nex << std::endl;
            std::cout << "remote_down_rank: " << remote_down_rank << std::endl;
            std::cout << "--------------------" << std::endl;

            return part_data;
        };

        //This example function shows how to a specific element of the struct
        //int get_mpi_rank(std::string part_id);
        int get_mpi_rank(std::string part_id)
        {
            //An example code for getting individual member of the struct identified by part_id
            partition_data part_data;
            part_data = partition_ranks.at(part_id);
            int mpi_world_rank = part_data.mpi_world_rank;

            std::cout << "\nmpi_world_rank: " << mpi_world_rank << std::endl;
            return mpi_world_rank;
        };

        std::unordered_map<std::string, partition_data> partition_ranks;


    private:
        int mpi_world_rank;
        std::vector<std::string> cat_ids;
        std::vector<std::string> nex_ids;
        std::string remote_up_nex;
        int remote_up_rank;
        std::string remote_down_nex;
        int remote_down_rank;

        //The following part is moved to the public section
        /*
        struct partition_data
        {
            int mpi_world_rank;
            std::vector<std::string> cat_ids;
            std::vector<std::string> nex_ids;
            std::string remote_up_nex;
            int remote_up_rank;
            std::string remote_down_nex;
            int remote_down_rank;
        };

        typedef partition_data part_strt;
        part_strt get_part_strt(std::string part_id);
        
        std::unordered_map<std::string, partition_data> partition_ranks;
        */

        boost::property_tree::ptree tree;
};

//#endif // NGEN_MPI_ACTIVE
#endif // Partions_Parser_H
