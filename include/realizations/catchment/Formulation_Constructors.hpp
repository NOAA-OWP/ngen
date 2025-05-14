#ifndef NGEN_FORMULATION_CONSTRUCTORS_H
#define NGEN_FORMULATION_CONSTRUCTORS_H
#include "Logger.hpp"

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
    using constructor = std::shared_ptr<Catchment_Formulation> (*)(std::string, std::shared_ptr<data_access::GenericDataProvider>, utils::StreamHandler);

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
        try {
            constructor formulation_constructor = formulations.at(formulation_type);

            // Log the forcing configuration being used
            std::cout << "[DEBUG] Forcing path: " << forcing_config.path << std::endl;
            std::cout << "[DEBUG] Forcing provider: " << forcing_config.provider << std::endl;
            std::cout << "[DEBUG] Forcing simulation start time: " << forcing_config.simulation_start_t << std::endl;
            std::cout << "[DEBUG] Forcing simulation end time: " << forcing_config.simulation_end_t << std::endl;

            std::shared_ptr<data_access::GenericDataProvider> fp;
            if (forcing_config.provider == "CsvPerFeature" || forcing_config.provider.empty()) {
                std::cout << "[DEBUG] Using CsvPerFeatureForcingProvider for identifier: " << identifier << std::endl;
                fp = std::make_shared<CsvPerFeatureForcingProvider>(forcing_config);
            }
    #if NGEN_WITH_NETCDF
            else if (forcing_config.provider == "NetCDF") {
                std::cout << "[DEBUG] Using NetCDFPerFeatureDataProvider for identifier: " << identifier << std::endl;
                fp = data_access::NetCDFPerFeatureDataProvider::get_shared_provider(forcing_config.path, forcing_config.simulation_start_t, forcing_config.simulation_end_t, output_stream);
            }
    #endif
            else if (forcing_config.provider == "NullForcingProvider") {
                std::cout << "[DEBUG] Using NullForcingProvider for identifier: " << identifier << std::endl;
                fp = std::make_shared<NullForcingProvider>();
            }
            else { // Some unknown string in the provider field?
                std::string throw_msg;
                throw_msg.assign(
                        "Invalid formulation forcing provider configuration! identifier: \"" + identifier +
                        "\", formulation_type: \"" + formulation_type +
                        "\", provider: \"" + forcing_config.provider + "\"");
                LOG(throw_msg, LogLevel::WARNING);
                std::cout << "[ERROR] " << throw_msg << std::endl;
                throw std::runtime_error(throw_msg);
            }

            // Log before calling the formulation constructor
            std::cout << "[DEBUG] Creating formulation for identifier: " << identifier
                      << ", type: " << formulation_type << std::endl;
            auto formulation = formulation_constructor(identifier, fp, output_stream);
//            std::cout << "[DEBUG] Formulation created successfully for identifier: " << identifier << std::endl;
            return formulation;
        }
        catch (const std::out_of_range &e) {
            std::string throw_msg = "No known formulation type '" + formulation_type + "' for identifier '" + identifier + "'";
            LOG(throw_msg, LogLevel::WARNING);
            std::cout << "[ERROR] " << throw_msg << std::endl;
            throw std::runtime_error(throw_msg);
        }
        catch (const std::exception &e) {
            std::string throw_msg = "Error constructing formulation for identifier '" + identifier + "': " + e.what();
            LOG(throw_msg, LogLevel::WARNING);
            std::cout << "[ERROR] " << throw_msg << std::endl;
            throw;
        }
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

        std::string throw_msg; throw_msg.assign("No valid formulation for " + *key + " was described in the passed in tree.");
        LOG(throw_msg, LogLevel::WARNING);
        throw std::runtime_error(throw_msg);
    }
}

#endif // NGEN_FORMULATION_CONSTRUCTORS_H
