#ifndef NGEN_REALIZATION_GLOBAL_CONFIG_H
#define NGEN_REALIZATION_GLOBAL_CONFIG_H

#include "JSONProperty.hpp"

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

} // namespace config
} // namespace realization

#endif // NGEN_REALIZATION_GLOBAL_CONFIG_H
