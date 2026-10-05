#ifndef NGEN_REALIZATION_CONFIG_AUXILIARY_ATTRIBUTES_HPP
#define NGEN_REALIZATION_CONFIG_AUXILIARY_ATTRIBUTES_HPP

#include <cstddef>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>

namespace realization {
  namespace config {

    //! Top-level realization config key holding the list of auxiliary attribute table declarations.
    static const std::string AUX_ATTRIBUTES_CONFIG_KEY = "auxiliary_hydrofabric_attributes";

    //! Key column assumed for an entry that does not name one.
    static const std::string DEFAULT_AUX_ATTRIBUTES_KEY_COLUMN = "divide_id";

    /**
     * One declared auxiliary attribute table: a table of per-divide values, living in a GeoPackage,
     * whose columns are joined onto the catchment features under a namespacing prefix.
     *
     * This is a pure configuration value; it performs no file or database I/O. Resolving @ref file
     * and reading the table itself, belong to the consumers of these entries.
     */
    struct AuxiliaryAttributeTable {
        //! Name of the attributes table in the GeoPackage. Required.
        std::string table;
        //! Short stand-in for @ref table when namespacing joined columns. Empty when not declared.
        std::string alias;
        //! GeoPackage holding @ref table. Empty means the catchment data file given on the CLI.
        std::string file;
        //! Column whose values are matched against catchment feature IDs.
        std::string key_column = DEFAULT_AUX_ATTRIBUTES_KEY_COLUMN;
        //! When true, a feature with no row in @ref table is an error rather than a warning.
        bool required = false;

        //! The namespace joined columns are published under: the alias when declared, else the table name.
        const std::string& prefix() const { return alias.empty() ? table : alias; }

        /**
         * Parse one entry of the @ref AUX_ATTRIBUTES_CONFIG_KEY list, leaving unstated fields at their
         * defaults. @p context identifies the entry in error messages (typically the key and a list index).
         *
         * @throw std::runtime_error if the entry is not an object, 'table' is absent, or a declared field
         *        has an unusable value.
         */
        static AuxiliaryAttributeTable from_tree(const boost::property_tree::ptree& entry,
                                                 const std::string& context)
        {
            if (entry.empty()) {
                throw std::runtime_error(context + ": entry must be an object declaring at least a 'table' name.");
            }

            AuxiliaryAttributeTable parsed;

            const boost::optional<const boost::property_tree::ptree&> table_node =
                entry.get_child_optional("table");
            if (!table_node) {
                throw std::runtime_error(context + ": required key 'table' is missing.");
            }
            parsed.table = scalar_string(*table_node, "table", context);

            if (const auto alias_node = entry.get_child_optional("alias")) {
                parsed.alias = scalar_string(*alias_node, "alias", context);
            }
            if (const auto file_node = entry.get_child_optional("file")) {
                parsed.file = scalar_string(*file_node, "file", context);
            }
            if (const auto key_column_node = entry.get_child_optional("key_column")) {
                parsed.key_column = scalar_string(*key_column_node, "key_column", context);
            }
            if (const auto required_node = entry.get_child_optional("required")) {
                try {
                    parsed.required = required_node->get_value<bool>();
                } catch (const boost::property_tree::ptree_bad_data&) {
                    throw std::runtime_error(context + ": 'required' must be a boolean.");
                }
            }

            return parsed;
        }

    private:
        //! Read the value of @p key as a non-empty scalar string, rejecting objects, lists and blanks.
        static std::string scalar_string(const boost::property_tree::ptree& node, const std::string& key,
                                         const std::string& context)
        {
            if (!node.empty()) {
                throw std::runtime_error(context + ": '" + key + "' must be a string, not an object or list.");
            }
            std::string value = node.get_value<std::string>();
            if (value.empty()) {
                throw std::runtime_error(context + ": '" + key + "' must not be empty.");
            }
            return value;
        }
    };

    /**
     * Parse the optional @ref AUX_ATTRIBUTES_CONFIG_KEY list from a realization config tree, in
     * configured order. An absent key yields an empty list. Prefixes (see
     * @ref AuxiliaryAttributeTable::prefix) must be unique across entries, since a repeated prefix would
     * let two tables' columns collide in the feature property map.
     *
     * @throw std::runtime_error if the key is not a list, an entry is malformed, or two entries resolve
     *        to the same prefix.
     */
    inline std::vector<AuxiliaryAttributeTable>
    parse_auxiliary_attributes(const boost::property_tree::ptree& realization_tree)
    {
        const boost::optional<const boost::property_tree::ptree&> node =
            realization_tree.get_child_optional(AUX_ATTRIBUTES_CONFIG_KEY);
        if (!node) {
            return {};
        }

        const std::string not_a_list = "'" + AUX_ATTRIBUTES_CONFIG_KEY + "' must be a list of table declarations.";
        if (node->empty() && !node->get_value<std::string>().empty()) {
            throw std::runtime_error(not_a_list);
        }

        std::vector<AuxiliaryAttributeTable> entries;
        std::set<std::string> prefixes;
        std::size_t index = 0;
        for (const auto& child : *node) {
            if (!child.first.empty()) {
                throw std::runtime_error(not_a_list);
            }

            const std::string context = AUX_ATTRIBUTES_CONFIG_KEY + "[" + std::to_string(index) + "]";
            AuxiliaryAttributeTable entry = AuxiliaryAttributeTable::from_tree(child.second, context);
            if (!prefixes.insert(entry.prefix()).second) {
                throw std::runtime_error(context + ": prefix '" + entry.prefix() + "' is already used by an earlier"
                                         " entry; give one of them a distinct 'alias'.");
            }

            entries.push_back(std::move(entry));
            ++index;
        }

        return entries;
    }

  }//end namespace config
}//end namespace realization
#endif // NGEN_REALIZATION_CONFIG_AUXILIARY_ATTRIBUTES_HPP
