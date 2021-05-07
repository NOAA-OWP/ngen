#ifndef Partions_Parser_H
#define Partions_Parser_H

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

#include "core/catchment/HY_CatchmentArea.hpp"

namespace realization {

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
        void read_partition_file(boost::property_tree::ptree &config);

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
        part_strt get_part_strt(std::string part_id);

        //This example function shows how to a specific element of the struct
        int get_mpi_rank(std::string part_id);

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
}

//#endif // NGEN_MPI_ACTIVE
#endif // Partions_Parser_H
