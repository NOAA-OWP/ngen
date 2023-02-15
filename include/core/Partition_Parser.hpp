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
#include <unordered_set>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <FeatureBuilder.hpp>
#include "features/Features.hpp"
#include <FeatureCollection.hpp>
#include "JSONProperty.hpp"

using Tuple = std::tuple<int, std::string, std::string, std::string>;

//This struct is moved from private section to here so that the unit test function can access it
struct PartitionData
{
    int mpi_world_rank;
    std::unordered_set<std::string> catchment_ids;
    std::unordered_set<std::string> nexus_ids;
    std::vector<Tuple> remote_connections;
};


class Partitions_Parser {

    public:
        //Constructor that takes an input json file path and points to the root of the tree in json file
        Partitions_Parser(const std::string &file_path) {
            boost::property_tree::ptree loaded_tree;
            boost::property_tree::json_parser::read_json(file_path, loaded_tree);
            std::cout << "file read success" << std::endl;
            this->tree = loaded_tree;
            std::cout << "file_path: " << file_path << std::endl;
        };

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
            int remote_mpi_rank;
            std::string remote_nex_id;
            std::string remote_cat_id;
            std::string direction;
            Tuple tmp_tuple;
            std::vector<Tuple> remote_conn_vec;
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

                if( part.at("remote-connections").get_type() == geojson::PropertyType::List ) {
                    //It is valid to have no remote connections, but the backend property tree parser
                    //can't represent empty lists/objects, so it turns into an ampty string (which is iterable)
                    //so we check to ensure the remote connections are a list type (not string) before we attempt
                    //to process the remote-connections.  If they are empty, this step gets skipped entirely.
                    //Get remote-connections and set the corresponding part_data struct member
                    
                for (auto &remote_conn : part.at("remote-connections").as_list())
                {
                    if ( remote_conn.get_type() != geojson::PropertyType::String )
                    {
                        remote_mpi_rank = remote_conn.at("mpi-rank").as_natural_number();
                        remote_nex_id = remote_conn.at("nex-id").as_string();
                        remote_cat_id = remote_conn.at("cat-id").as_string();
                        direction = remote_conn.at("cat-direction").as_string();
                        tmp_tuple = std::make_tuple(remote_mpi_rank, remote_nex_id, remote_cat_id, direction);
                        remote_conn_vec.push_back(tmp_tuple);
                    }
                }
                part_data.remote_connections = remote_conn_vec;
                remote_conn_vec.clear();

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
             * The function takes a partition id and return the corresponding struct from the set and remote_connections.
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

            for (auto i = part_data.remote_connections.cbegin(); i != part_data.remote_connections.cend(); ++i)
            {
                std::tuple<int, std::string, std::string> remote_conn = *i;
                int mpi_rank = std::get<0>(remote_conn);
                std::string nex_id = std::get<1>(remote_conn);
                std::string cat_id = std::get<2>(remote_conn);

                std::cout << "\nget_partition_struct, remote_mpi_ranks: " << mpi_rank; 
                std::cout << "\nget_partition_struct, remote_nexus: " << nex_id;
                std::cout << "\nget_partition_struct, remote_catchment: " << cat_id;
            }
            std::cout << "\n--------------------" << std::endl;
            */

            return part_data;
        };

        //This example function shows how to get a specific member of the struct
        int get_mpi_rank(int part_id)
        {
            //An example code for getting individual member of the struct identified by part_id
            PartitionData part_data;
            part_data = partition_ranks[part_id];
            int mpi_world_rank = part_data.mpi_world_rank;

            std::cout << "mpi_world_rank: " << mpi_world_rank << std::endl;
            return mpi_world_rank;
        };

        // partition_ranks is a vector of struct: PartitionData
        std::vector<PartitionData> partition_ranks;

    private:
        int mpi_world_rank;
        std::unordered_set<std::string> catchment_ids;
        std::unordered_set<std::string> nexus_ids;
        std::vector<std::tuple<int, std::string, std::string> > remote_connections;
        std::tuple<int, std::string, std::string> remote_tuple;

        boost::property_tree::ptree tree;
};

#endif // NGEN_MPI_ACTIVE
#endif // PARTITION_PARSER_H
