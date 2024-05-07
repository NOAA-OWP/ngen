#ifndef NGEN_FORMULATION_CONSTRUCTORS_H
#define NGEN_FORMULATION_CONSTRUCTORS_H

#include <NGenConfig.h>

#include "Formulation.hpp"
#include <JSONProperty.hpp>
#include <exception>

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>

// Formulations
#include "Bmi_Formulation.hpp"
#include <GenericDataProvider.hpp>
#include "CsvPerFeatureForcingProvider.hpp"
#include "NullForcingProvider.hpp"
#if NGEN_WITH_NETCDF
    #include "NetCDFPerFeatureDataProvider.hpp"
#endif

namespace realization {
    using constructor = std::shared_ptr<Catchment_Formulation> (*)(std::string, shared_ptr<data_access::GenericDataProvider>, utils::StreamHandler);

    extern std::map<std::string, constructor> formulations;

    static std::string valid_formulation_keys(){
        std::string keys = "";
        for(const auto& kv : formulations){
            keys.append(kv.first+" ");
        }
        return keys;
    }

    static bool formulation_exists(std::string formulation_type) {
        return formulations.count(formulation_type) > 0;
    }

    static std::shared_ptr<Catchment_Formulation> construct_formulation(
        std::string formulation_type,
        std::string identifier,
        forcing_params &forcing_config,
        utils::StreamHandler output_stream
    ) {
        constructor formulation_constructor = formulations.at(formulation_type);
        std::shared_ptr<data_access::GenericDataProvider> fp;
        if (forcing_config.provider == "CsvPerFeature" || forcing_config.provider == ""){
            fp = std::make_shared<CsvPerFeatureForcingProvider>(forcing_config);
        }
#if NGEN_WITH_NETCDF
        else if (forcing_config.provider == "NetCDF"){
            fp = data_access::NetCDFPerFeatureDataProvider::get_shared_provider(forcing_config.path, forcing_config.simulation_start_t, forcing_config.simulation_end_t, output_stream);
        }
#endif
        else if (forcing_config.provider == "NullForcingProvider"){
            fp = std::make_shared<NullForcingProvider>();
        }
        else { // Some unknown string in the provider field?
            throw std::runtime_error(
                    "Invalid formulation forcing provider configuration! identifier: \"" + identifier +
                    "\", formulation_type: \"" + formulation_type +
                    "\", provider: \"" + forcing_config.provider + "\"");
        }
        return formulation_constructor(identifier, fp, output_stream);
    }

    static std::string get_formulation_key(const boost::property_tree::ptree &tree) {
        /*for (auto &node : tree) {
            if (formulation_exists(node.first)) {
                return node.first;
            }
        }*/
        boost::optional<std::string> key = tree.get_optional<std::string>("name");
        if(key && formulation_exists(*key)){
          return *key;
        }

        throw std::runtime_error("No valid formulation for " + *key + " was described in the passed in tree.");
    }
}

#endif // NGEN_FORMULATION_CONSTRUCTORS_H
