#ifndef NGEN_REALIZATION_CONFIG_PROPERTIES_H
#define NGEN_REALIZATION_CONFIG_PROPERTIES_H

#include <string>

#include "JSONProperty.hpp"

namespace realization{
  namespace config{

    /**
     * Perform in-place substitution on the given config property item, if the item and the pattern
     * are present.
     *
     * Any and all instances of the substring ``pattern`` are replaced by ``replacement``, if
     * ``key`` maps to a present string-type property value.
     *
     * @param properties A reference to the properties config object to be altered.
     * @param key The key for the configuration property to potentially adjust.
     * @param pattern The pattern substring to search for that, when present, should be replaced.
     * @param replacement The replacement substring to potentially insert.
     */
    inline void pattern_substitution(geojson::PropertyMap &properties, const std::string &key,
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

  }//end namespace config
}//end namespace realization

#endif // NGEN_REALIZATION_CONFIG_PROPERTIES_H
