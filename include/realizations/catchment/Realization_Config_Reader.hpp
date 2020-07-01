#ifndef REALIZATION_CONFIG_READER_H
#define REALIZATION_CONFIG_READER_H

#include <memory>
#include <sstream>
#include <string>
#include <map>
#include <exception>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "Realization_Config.hpp"

namespace realization {
    class Raw_Realization_Config_Reader;

    typedef std::shared_ptr<Raw_Realization_Config_Reader> Realization_Config_Reader;

    class Raw_Realization_Config_Reader {
        public:
            Raw_Realization_Config_Reader(boost::property_tree::ptree &tree) : tree(tree) {}

            void read();

            Realization_Config get_global_configuration() {
                return this->global_realization_config;
            }

            /**
             * Destructor
             */
            virtual ~Raw_Realization_Config_Reader(){};

            Realization_Config get(std::string identifier) const;

            bool contains(std::string identifier) const;

            /**
             * @return The number of elements within the collection
             */
            int get_size();

            /**
             * @return Whether or not the collection is empty
             */
            bool is_empty();

            std::map<std::string, Realization_Config>::const_iterator begin() const;

            std::map<std::string, Realization_Config>::const_iterator end() const;

        private:
            boost::property_tree::ptree tree;
            std::map<std::string, realization::Realization_Config> realization_configs;
            realization::Realization_Config global_realization_config = nullptr;
    };


    static Realization_Config_Reader load_reader(const std::string &file_path) {
        boost::property_tree::ptree tree;
        boost::property_tree::json_parser::read_json(file_path, tree);
        return std::make_shared<Raw_Realization_Config_Reader>(Raw_Realization_Config_Reader(tree));
    }

    static Realization_Config_Reader load_reader(std::stringstream &data) {
        boost::property_tree::ptree tree;
        boost::property_tree::json_parser::read_json(data, tree);
        return std::make_shared<Raw_Realization_Config_Reader>(Raw_Realization_Config_Reader(tree));
    }
}

#endif // REALIZATION_CONFIG_READER_H