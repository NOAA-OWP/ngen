#include <Formulation_Manager.hpp>

#if NGEN_WITH_NETCDF
    #include "NetCDFPerFeatureDataProvider.hpp"
#endif

using namespace realization;

void Formulation_Manager::finalize() {
    // The calls in these loops are staticly dispatched to
    // Catchment_Formulation::finalize(). That does not
    // inherit from DataProvider, with its virtual member
    // function of the same name.
    //
    // If any formulation class needs to customize this
    // behavior through this becoming a virtual dispatch,
    // take care. Bmi_Multi_Formulation was a concern, but
    // does not currently need to because none of its
    // constituent formulations points to any forcing
    // object other than the enclosing
    // Bmi_Multi_Formulation instance itself.
    for (auto const& fmap: formulations) {
        fmap.second->finalize();
    }
    for (auto const& fmap: domain_formulations) {
        fmap.second->finalize();
    }

#if NGEN_WITH_NETCDF
    data_access::NetCDFPerFeatureDataProvider::cleanup_shared_providers();
#endif
}

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
