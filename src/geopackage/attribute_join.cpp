#include "attribute_join.hpp"
#include "geopackage.hpp"
#include "JSONProperty.hpp"
#include "logging_utils.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

//! Open the GeoPackage an attribute table lives in, naming that table if the open fails.
ngen::sqlite::database open_for_table(const std::string& gpkg_path, const std::string& table)
{
    try {
        return ngen::sqlite::database{gpkg_path};
    } catch (const std::exception& error) {
        throw std::runtime_error(
            "cannot open " + gpkg_path + ", declared as the source of table `" +
            table + "`: " + error.what()
        );
    }
}

//! How many unmatched feature IDs a coverage report names before falling back to a count.
constexpr std::size_t MAX_UNMATCHED_NAMED = 10;

//! Whether a cell of this SQLite type has a JSON property counterpart.
bool is_convertible_type(int type)
{
    return type == SQLITE_INTEGER || type == SQLITE_FLOAT || type == SQLITE_TEXT;
}

} // anonymous namespace

void ngen::geopackage::join_attributes(
    geojson::FeatureCollection& collection,
    const std::string& gpkg_path,
    const AttributeJoinSpec& spec
)
{
    const std::string table_identifier = quote_table_name(spec.table);
    ngen::sqlite::database db = open_for_table(gpkg_path, spec.table);

    // An absent table or key column is a typo rather than a data gap, so `required` does not enter into it.
    if (!db.contains(spec.table)) {
        throw std::runtime_error(
            "table `" + spec.table + "` does not exist in " + gpkg_path
        );
    }

    auto rows = db.query("SELECT * FROM " + table_identifier);
    const int key_index = rows.find(spec.key_column);
    if (key_index < 0) {
        throw std::runtime_error(
            "table `" + spec.table + "` in " + gpkg_path +
            " has no key column `" + spec.key_column + "`"
        );
    }

    // A collection is not required to hold one feature per id, so an id's row joins onto each of them.
    std::unordered_map<std::string, std::vector<geojson::Feature>> features_by_id;
    for (const auto& feature : collection) {
        features_by_id[feature->get_id()].push_back(feature);
    }

    const auto columns = rows.columns();
    std::unordered_set<std::string> joined_ids;
    rows.next();
    while (!rows.done()) {
        const std::string key = rows.get<std::string>(key_index);
        const auto found = features_by_id.find(key);
        if (found != features_by_id.end()) {
            // Nothing in the table says which of two rows keyed the same holds the feature's value,
            // so taking whichever a scan reaches first makes it an artifact of the file's layout.
            if (!joined_ids.insert(key).second) {
                throw std::runtime_error(
                    "table `" + spec.table + "` in " + gpkg_path +
                    " has more than one row keyed `" + key + "`"
                );
            }

            const auto types = rows.types();
            for (const geojson::Feature& feature : found->second) {
                geojson::PropertyMap& properties = feature->get_properties();

                for (std::size_t i = 0; i < columns.size(); i++) {
                    // A NULL cell means the table has no value here, not that the value is null;
                    // a cell of any other unconvertible type has none to publish either.
                    if (static_cast<int>(i) == key_index || !is_convertible_type(types[i])) {
                        continue;
                    }

                    const std::string name = spec.prefix() + "." + columns[i];
                    const bool inserted = properties.emplace(
                        name, geojson::JSONProperty(name, get_property(rows, columns[i], types[i]))
                    ).second;

                    // Keeping the property already there would hand the model another source's
                    // value under this table's name.
                    if (!inserted) {
                        throw std::runtime_error(
                            "joining table `" + spec.table + "` of " + gpkg_path +
                            " onto feature `" + key + "` would overwrite its existing property `" +
                            name + "`"
                        );
                    }
                }
            }
        }

        rows.next();
    }

    // A table covering only part of a collection is worth one report, not one per feature: a
    // regionalization table missing a handful of rows out of thousands would otherwise bury the
    // run's output. Naming the first few keeps the common case -- a gap of one or two -- diagnosable
    // without opening the file.
    std::vector<std::string> unmatched;
    std::unordered_set<std::string> reported_ids;
    for (const auto& feature : collection) {
        const std::string& id = feature->get_id();
        if (joined_ids.contains(id) || !reported_ids.insert(id).second) {
            continue;
        }
        unmatched.push_back(id);
    }

    if (unmatched.empty()) {
        return;
    }

    const std::size_t named = std::min(unmatched.size(), MAX_UNMATCHED_NAMED);
    std::string message = std::to_string(unmatched.size()) +
                          (unmatched.size() == 1 ? " feature has" : " features have") +
                          " no row in table `" + spec.table + "` of " + gpkg_path + ": ";
    for (std::size_t i = 0; i < named; i++) {
        message += (i == 0 ? "" : ", ") + unmatched[i];
    }
    if (named < unmatched.size()) {
        message += ", and " + std::to_string(unmatched.size() - named) + " more";
    }

    if (spec.required) {
        throw std::runtime_error(message);
    }

    logging::warning((message + "\n").c_str());
}

void ngen::geopackage::join_all(
    geojson::FeatureCollection& collection,
    const std::vector<AttributeJoinSpec>& specs,
    const std::string& default_source,
    const bool default_source_is_gpkg,
    const std::string& context
)
{
    for (std::size_t index = 0; index < specs.size(); index++) {
        const AttributeJoinSpec& spec = specs[index];

        // Entries are identified by position, the way the parser reporting on them does. Naming the
        // table here instead would repeat what the joiner's own message already says, and a table
        // may legitimately be declared twice from two files, so position is what tells them apart.
        const std::string entry = context + "[" + std::to_string(index) + "]";

        if (spec.file.empty() && !default_source_is_gpkg) {
            throw std::runtime_error(
                entry + ": table '" + spec.table + "' declares no 'file', but '" + default_source +
                "' is not a GeoPackage."
            );
        }

        try {
            join_attributes(collection, spec.file.empty() ? default_source : spec.file, spec);
        } catch (const std::exception& error) {
            // The joiner knows the table and the file, not which declaration asked for them.
            throw std::runtime_error(entry + ": " + error.what());
        }
    }
}
