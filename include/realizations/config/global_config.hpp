#ifndef NGEN_REALIZATION_GLOBAL_CONFIG_H
#define NGEN_REALIZATION_GLOBAL_CONFIG_H

#include "JSONProperty.hpp"

#include <string>
#include <utility>
#include <vector>

namespace realization {
namespace config {

/**
 * @brief Identifier for a realization-level configuration block that
 *        can be inherited down into each formulation's params.
 *
 * The inheritance convention is: if a realization-level JSON document
 * carries a block under the key identified by this enum, every
 * formulation built by `Formulation_Manager` receives that block in
 * its params unless the per-formulation config already declares it.
 * Within a multi-BMI formulation, the block is further propagated
 * into each submodule's params on the same "child wins" basis.
 *
 * Adding a new realization-level inheritable block is a two-step:
 *   1) add an enum entry here;
 *   2) add a matching `case` in `to_key_string()` below.
 * The rest of the plumbing (parsing, injection, submodule propagation)
 * is already wired to iterate enum values.
 */
enum class GlobalConfigKey {
    /** The `serialization` block consumed by the BMI serialization
     *  protocols (Ngen{Serialization,Deserialization}Protocol). */
    SERIALIZATION,
};

/** @brief Top-level JSON key string for a given GlobalConfigKey. */
inline const char* to_key_string(GlobalConfigKey which) {
    switch (which) {
        case GlobalConfigKey::SERIALIZATION: return "serialization";
    }
    // Enum is exhaustively handled above; any future addition must
    // supply a case or compilation should fail at the call site.
    return "";
}

/**
 * @brief Copy a named config block from @p parent into @p child,
 *        unless the child already declares its own entry for the same
 *        key.
 *
 * Per-child entries always win; the parent value is a default, not
 * an override. This is the single mechanism for realization-level
 * config inheritance — `Formulation_Manager` calls it to seed every
 * formulation from parsed top-level blocks, and
 * `Bmi_Multi_Formulation` calls it during submodule setup to carry
 * the same defaults further down the tree.
 *
 * No-op if the parent has no entry under @p which.
 */
inline void apply_config(geojson::PropertyMap&       child,
                         const geojson::PropertyMap& parent,
                         GlobalConfigKey             which) {
    const char* key = to_key_string(which);
    auto src_it = parent.find(key);
    if (src_it == parent.end()) return;
    if (child.count(key) != 0) return;
    child.emplace(key, src_it->second);
}

/**
 * @brief Auto-populate `serialization.restore.id_subset` with the set
 *        of catchment identifiers that this realization will actually
 *        instantiate, when the user hasn't supplied one.
 *
 * Why — at scale the `CheckpointIndex` memory footprint is proportional
 * to the number of records it indexes. A run that only needs records
 * for the local rank's partition shouldn't pay the memory cost of
 * indexing every record in the shared checkpoint file. The protocol
 * already honors `id_subset` as a manual knob; this helper fills it in
 * automatically so the optimization applies without requiring the
 * caller to enumerate features by hand.
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

#endif // NGEN_REALIZATION_GLOBAL_CONFIG_H
