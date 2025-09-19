#include <Catchment_Formulation.hpp>


namespace realization {

    Catchment_Formulation::Catchment_Formulation(std::string id,
                                                 std::shared_ptr<data_access::GenericDataProvider> forcing,
                                                 utils::StreamHandler output_stream)
        : Formulation(id)
        , HY_CatchmentArea(output_stream)
        , forcing(forcing)
    {
        // Assume the catchment ID is equal to or embedded in the formulation `id`
        size_t idx = id.find(".");
        set_catchment_id( idx == std::string::npos ? id : id.substr(0, idx) );
    }

    Catchment_Formulation::Catchment_Formulation(std::string id)
        : Formulation(id)
    {
        // Assume the catchment ID is equal to or embedded in the formulation `id`
        size_t idx = id.find(".");
        set_catchment_id( idx == std::string::npos ? id : id.substr(0, idx) );
    }

    void Catchment_Formulation::config_pattern_substitution(geojson::PropertyMap &properties, const std::string &key,
                                                            const std::string &pattern, const std::string &replacement)
    {
        auto it = properties.find(key);
        // Do nothing and return if either the key isn't found or the associated property isn't a string
        if (it == properties.end() || it->second.get_type() != geojson::PropertyType::String) {
            std::cout << "[DEBUG] Skipping pattern substitution for key: " << key << " (not found or not a string)" << std::endl;
            return;
        }

        std::string value = it->second.as_string();
        std::cout << "[DEBUG] config_pattern_substitution Performing pattern substitution for key: " << key
                  << ", pattern: " << pattern << ", replacement: " << replacement << std::endl;
        //            std::cout << "[DEBUG] Original value: " << value << std::endl;

        size_t id_index = value.find(pattern);
        while (id_index != std::string::npos) {
            value = value.replace(id_index, pattern.size(), replacement);
            id_index = value.find(pattern);
        }

        // Update the property with the substituted value
        properties.erase(key);
        properties.emplace(key, geojson::JSONProperty(key, value));

        //            std::cout << "[DEBUG] Substitution result for key: " << key << " -> " << value << std::endl;
    }

    std::string Catchment_Formulation::get_output_header_line(std::string delimiter) const {
        return "Total Discharge";
    }

    void Catchment_Formulation::finalize()
    {
        if (forcing) {
            forcing->finalize();
            forcing = nullptr;
        }
    }
} // namespace realization
