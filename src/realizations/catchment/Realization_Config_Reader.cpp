#include "Realization_Config_Reader.hpp"

#include "Realization_Config.hpp"

#include <boost/property_tree/ptree.hpp>

using namespace realization;

void Raw_Realization_Config_Reader::read() {
    auto possible_global_config = tree.get_child_optional("global");

    if (possible_global_config) {
        this->global_realization_config = get_realizationconfig("all", *possible_global_config);
    }

    auto possible_catchment_configs = tree.get_child_optional("catchments");

    if (possible_catchment_configs) {
        for (auto &config_pair : *possible_catchment_configs) {
            Realization_Config new_config = get_realizationconfig(config_pair.first, config_pair.second, this->global_realization_config);
            this->realization_configs.emplace(config_pair.first, new_config);
        }
    }
}

std::map<std::string, Realization_Config>::const_iterator Raw_Realization_Config_Reader::begin() const {
    return this->realization_configs.cbegin();
}

std::map<std::string, Realization_Config>::const_iterator Raw_Realization_Config_Reader::end() const {
    return this->realization_configs.cend();
}


Realization_Config Raw_Realization_Config_Reader::get(std::string identifier) const {
    if (this->contains(identifier)) {
        return this->realization_configs.at(identifier);
    }
    else if (this->global_realization_config != nullptr) {
        return this->global_realization_config->assign_id(identifier);
    }

    std::string message = "This configuration reader cannot find the identifier '" + identifier + "'";
    throw std::runtime_error(message);
}

bool Raw_Realization_Config_Reader::contains(std::string identifier) const {
    // Maps can only have a single value per key, so if the map contains the value, it'll be 1, otherwise 0
    return this->realization_configs.count(identifier) == 1;
}

/**
 * @return The number of elements within the collection
 */
int Raw_Realization_Config_Reader::get_size() {
    return this->realization_configs.size();
}

/**
 * @return Whether or not the collection is empty
 */
bool Raw_Realization_Config_Reader::is_empty() {
    return this->get_size() == 0;
}