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
            return;
        }

        std::string value = it->second.as_string();

        size_t id_index = value.find(pattern);

        if (id_index != std::string::npos) {
            do {
                value = value.replace(id_index, sizeof(pattern.c_str()) - 2, replacement);
                id_index = value.find(pattern);
            } while (id_index != std::string::npos);

            properties.erase(key);
            properties.emplace(key, geojson::JSONProperty(key, value));
        }
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
