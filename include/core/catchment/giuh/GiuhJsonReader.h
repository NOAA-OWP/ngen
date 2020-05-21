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
        GiuhJsonReader(std::string file_path) : json_file_path(file_path) {
            // Confirm file exists and is readable
            if (FILE *file = fopen(file_path.c_str(), "r")) {
                fclose(file);
                file_readable = true;
                ptree tree;
                boost::property_tree::json_parser::read_json(json_file_path, tree);
                json_tree = std::make_unique<ptree>(tree);
            }
            else {
                file_readable = false;
                json_tree = nullptr;
            }
        }

        std::shared_ptr<giuh_kernel> get_giuh_kernel_for_id(std::string catchment_id);

        bool is_giuh_kernel_for_id_exists(std::string catchment_id);

        bool is_json_file_readable();

    private:
        bool file_readable;
        std::string json_file_path;
        std::unique_ptr<ptree> json_tree;

        std::unique_ptr<ptree> find_data_node_for_catchment_id(std::string catchment_id);

        std::shared_ptr<giuh_kernel> build_giuh_kernel(std::string catchment_id, ptree catchment_data_node);

    };

}

#endif //NGEN_GIUHJSONREADER_H
