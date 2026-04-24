#ifndef NGEN_REALIZATION_SERIALIZATION_HELPER_H
#define NGEN_REALIZATION_SERIALIZATION_HELPER_H

#include "JSONProperty.hpp"
#include "global_config.hpp"

#include <string>
#include <vector>

namespace realization {
namespace config {

/**
 * @brief Auto-populate `serialization.restore.id_subset` with the set
 *        of catchment identifiers that this realization will actually
 *        instantiate, when the user hasn't supplied one.
 *
 * The serialization restore protocol honors `id_subset` as a manual
 * knob to bound the restore lookup's scope; this helper fills it in
 * automatically so the scoping applies without requiring the caller
 * to enumerate features by hand.
 *
 * No-op if any of the following hold (deliberately conservative):
 *   - no `serialization` block is present in @p global_configs;
 *   - the serialization block has no `restore` sub-block;
 *   - the `restore` sub-block already declares `id_subset` — the
 *     caller's explicit value always wins, same as the rest of the
 *     inheritance system;
 *   - @p known_ids is empty (passing an empty list could be read as
 *     "restore nothing", which is the wrong default).
 *
 * @param global_configs  The realization-level inheritable configs map
 *                        (mutated in place).
 * @param known_ids       Catchment ids the caller intends to construct.
 */
inline void apply_serialization_restore_subset_default(
    geojson::PropertyMap&              global_configs,
    const std::vector<std::string>&    known_ids)
{
    if (known_ids.empty()) return;

    const char* ser_key = to_key_string(GlobalConfigKey::SERIALIZATION);
    auto ser_it = global_configs.find(ser_key);
    if (ser_it == global_configs.end()) return;
    if (ser_it->second.get_type() != geojson::PropertyType::Object) return;

    geojson::PropertyMap ser_map = ser_it->second.get_values();

    auto restore_it = ser_map.find("restore");
    if (restore_it == ser_map.end()) return;
    if (restore_it->second.get_type() != geojson::PropertyType::Object) return;

    geojson::PropertyMap restore_map = restore_it->second.get_values();
    if (restore_map.count("id_subset") != 0) return;  // caller already set it

    // Build the id_subset list as a List-typed JSONProperty whose
    // elements are string-typed JSONProperties (matching how the
    // ptree-based constructor emits JSON arrays of strings).
    std::vector<geojson::JSONProperty> id_list;
    id_list.reserve(known_ids.size());
    for (const auto& id : known_ids) {
        id_list.emplace_back(std::string(""), id);
    }
    restore_map.emplace(
        "id_subset",
        geojson::JSONProperty(std::string("id_subset"), std::move(id_list))
    );

    // Rebuild the nested objects with the new map. JSONProperty's
    // PropertyMap-ctor takes a non-const reference and captures the
    // map by value into its internal field, so these locals are the
    // actual owners of the reconstituted tree.
    geojson::JSONProperty restore_prop(std::string("restore"), restore_map);
    ser_map.erase("restore");
    ser_map.emplace("restore", std::move(restore_prop));

    geojson::JSONProperty ser_prop(std::string(ser_key), ser_map);
    global_configs.erase(ser_key);
    global_configs.emplace(ser_key, std::move(ser_prop));
}

} // namespace config
} // namespace realization

#endif // NGEN_REALIZATION_SERIALIZATION_HELPER_H
