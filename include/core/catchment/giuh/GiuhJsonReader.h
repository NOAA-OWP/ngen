#ifndef NGEN_GIUHJSONREADER_H
#define NGEN_GIUHJSONREADER_H

#include "GIUH.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <memory>
#include <string>
#include <vector>


namespace giuh {

    typedef boost::property_tree::ptree ptree;

    class GiuhJsonReader {

    public:
        GiuhJsonReader(std::string data_file_path) : data_json_file_path(data_file_path) {
            // Confirm file exists and is readable
            if (FILE *file = fopen(data_file_path.c_str(), "r")) {
                fclose(file);
                data_json_file_readable = true;
                ptree tree;
                boost::property_tree::json_parser::read_json(data_json_file_path, tree);
                data_json_tree = std::make_unique<ptree>(tree);
            }
            else {
                data_json_file_readable = false;
                data_json_tree = nullptr;
            }
        }

        std::shared_ptr<giuh_kernel> get_giuh_kernel_for_id(std::string catchment_id);

        bool is_giuh_kernel_for_id_exists(std::string catchment_id);

        /**
         * Get whether a file at the path represented by `data_json_file_path` exists and is readable.
         *
         * @return Whether a file at the path represented by `data_json_file_path` exists and is readable.
         */
        bool is_data_json_file_readable();

    private:
        /** Whether a file at the path represented by `data_json_file_path` exists and is readable. */
        bool data_json_file_readable;
        /** The path to the JSON file containing GIUH data, as a string. */
        std::string data_json_file_path;
        std::unique_ptr<ptree> data_json_tree;

        std::unique_ptr<ptree> find_data_node_for_catchment_id(std::string catchment_id);

        std::shared_ptr<giuh_kernel> build_giuh_kernel(std::string catchment_id, ptree catchment_data_node);

    };

}

#endif //NGEN_GIUHJSONREADER_H
