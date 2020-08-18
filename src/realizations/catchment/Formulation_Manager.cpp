#include <Formulation_Manager.hpp>

using namespace realization;
/*
template<class realization_type>
Realization_Manager<realization_type>::Realization_Manager(std::stringstream &data) {

}

template<class realization_type>
Realization_Manager<realization_type>::Realization_Manager(const std::string &file_path) {

}

template<class realization_type>
void Realization_Manager<realization_type>::read() {
    auto possible_global_config = tree.get_child_optional("global");

    realization_type* global_realization = nullptr;

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

template<class realization_type>
void Realization_Manager<realization_type>::add_realization(realization_type realization) {
    this->realizations.emplace(realization.get_id(), realization);
}

template<class realization_type>
realization_type Realization_Manager<realization_type>::get_realization(std::string id) const {
    return this->realizations.at(id);
}

template<class realization_type>
bool Realization_Manager<realization_type>::contains(std::string identifier) const {
    return this->realizations.find(identifier) > 0;
}

template<class realization_type>
int Realization_Manager<realization_type>::get_size() {
    return this->realizations.size();
}

template<class realization_type>
bool Realization_Manager<realization_type>::is_empty() {
    return this->realizations.empty();
}

template<class realization_type>
typename std::map<std::string, realization_type>::const_iterator Realization_Manager<realization_type>::begin() const {
    return this->realizations.cbegin();
}

template<class realization_type>
typename std::map<std::string, realization_type>::const_iterator Realization_Manager<realization_type>::end() const {
    return this->realizations.cend();
}*/