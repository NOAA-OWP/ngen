#ifndef NGEN_GIUHJSONREADER_H
#define NGEN_GIUHJSONREADER_H

#include "all.h"
#include "GIUH.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <map>

namespace giuh {

    typedef boost::property_tree::ptree ptree;

    class GiuhJsonReader {

    public:
        GiuhJsonReader(std::string data_file_path, std::string id_map_path)
            : data_json_file_path(std::move(data_file_path)), id_map_json_file_path(std::move(id_map_path))
        {
            // Confirm data JSON file exists and is readable
            if (FILE *file = fopen(data_json_file_path.c_str(), "r")) {
                fclose(file);
                data_json_file_readable = true;
                ptree tree;
                boost::property_tree::json_parser::read_json(data_json_file_path, tree);
                data_json_tree = std::make_unique<ptree>(tree);
            } else {
                data_json_file_readable = false;
                data_json_tree = nullptr;
            }

            // Do the same for the id mapping data
            if (FILE *file = fopen(id_map_json_file_path.c_str(), "r")) {
                fclose(file);
                id_map_json_file_readable = true;
                id_map = std::make_unique<std::map<std::string, std::string>>(std::map<std::string, std::string>());

                ptree id_map_tree;
                boost::property_tree::json_parser::read_json(id_map_json_file_path, id_map_tree);

                for (ptree::iterator pos = id_map_tree.begin(); pos != id_map_tree.end(); ++pos) {
                    //std::string local_id = pos->second.get<std::string>("local_id");
                    std::string comid = pos->second.get<std::string>("COMID");
                    id_map->emplace(pos->first, comid);
                }
            } else {
                id_map_json_file_readable = false;
                //id_map_json_tree = nullptr;
            }
        }

        /**
         * Extract the ordinate values from the deserialized JSON data.
         *
         * @param catchment_data_node
         * @return
         */
        std::vector<double> extract_cumulative_frequency_ordinates(ptree catchment_data_node);

        /**
         * Extract the ordinate values, getting the appropriate JSON node via lookup
         *
         * @param catchment_id
         * @return
         */
        std::vector<double> extract_cumulative_frequency_ordinates(std::string catchment_id);

        std::string get_associated_comid(std::string catchment_id);

        std::shared_ptr<giuh_kernel_impl> get_giuh_kernel_for_id(std::string catchment_id);

        bool is_giuh_kernel_for_id_exists(std::string catchment_id);

        /**
         * Get whether a file at the path represented by `data_json_file_path` exists and is readable.
         *
         * @return Whether a file at the path represented by `data_json_file_path` exists and is readable.
         */
        bool is_data_json_file_readable();

        /**
         * Get whether a file at the path represented by `id_map_json_file_path` exists and is readable.
         *
         * @return Whether a file at the path represented by `id_map_json_file_path` exists and is readable.
         */
        bool is_id_map_json_file_readable();

    private:
        // TODO: document tree structure for both data and id mapping ptrees

        /** Whether a file at the path represented by `data_json_file_path` exists and is readable. */
        bool data_json_file_readable;
        /** The path to the JSON file containing GIUH data, as a string. */
        std::string data_json_file_path;
        /** A ptree representation of the GIUH data from the `data_json_file_path` file. */
        std::unique_ptr<ptree> data_json_tree;
        /** Whether a file at the path represented by `id_map_json_file_path` exists and is readable. */
        bool id_map_json_file_readable;
        /** The path to the JSON file containing map of catchment ids to COMIDs, as a string. */
        std::string id_map_json_file_path;
        /** A ptree representation of the id mapping data from the `id_map_json_file_path` file. */
        //std::unique_ptr<ptree> id_map_json_tree;

        std::unique_ptr<std::map<std::string, std::string>> id_map;



        std::unique_ptr<ptree> find_data_node_for_comid(std::string comid);

        std::string get_mapped_comid(std::string catchment_id);

        std::shared_ptr<giuh_kernel_impl> build_giuh_kernel(std::string catchment_id, std::string comid,
                                                            ptree catchment_data_node);

    };

}

#endif //NGEN_GIUHJSONREADER_H
