#include "geopackage.hpp"
#include "JSONProperty.hpp"
#include "logging_utils.h"

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
            "cannot open " + gpkg_path + ", declared as the source of auxiliary attribute table `" +
            table + "`: " + error.what()
        );
    }
}

//! Whether a cell of this SQLite type has a JSON property counterpart.
bool is_convertible_type(int type)
{
    return type == SQLITE_INTEGER || type == SQLITE_FLOAT || type == SQLITE_TEXT;
}

} // anonymous namespace

void ngen::geopackage::join_attributes(
    geojson::FeatureCollection& collection,
    const std::string& gpkg_path,
    const std::string& table,
    const std::string& key_column,
    const std::string& prefix,
    bool required
)
{
    const std::string table_identifier = quote_table_name(table);
    ngen::sqlite::database db = open_for_table(gpkg_path, table);

    // An absent table or key column is a typo rather than a data gap, so `required` does not enter into it.
    if (!db.contains(table)) {
        throw std::runtime_error(
            "auxiliary attribute table `" + table + "` does not exist in " + gpkg_path
        );
    }

    ngen::sqlite::database::iterator rows = db.query("SELECT * FROM " + table_identifier);
    const int key_index = rows.find(key_column);
    if (key_index < 0) {
        throw std::runtime_error(
            "auxiliary attribute table `" + table + "` in " + gpkg_path +
            " has no key column `" + key_column + "`"
        );
    }

    // A collection is not required to hold one feature per id, so an id's row joins onto each of them.
    std::unordered_map<std::string, std::vector<geojson::Feature>> features_by_id;
    for (const auto& feature : collection) {
        features_by_id[feature->get_id()].push_back(feature);
    }

    const boost::span<const std::string> columns = rows.columns();
    std::unordered_set<std::string> joined_ids;
    rows.next();
    while (!rows.done()) {
        const std::string key = rows.get<std::string>(key_index);
        const auto found = features_by_id.find(key);
        if (found != features_by_id.end()) {
            // Nothing in the table says which of two rows keyed the same holds the divide's value,
            // so taking whichever a scan reaches first makes it an artifact of the file's layout.
            if (!joined_ids.insert(key).second) {
                throw std::runtime_error(
                    "auxiliary attribute table `" + table + "` in " + gpkg_path +
                    " has more than one row keyed `" + key + "`"
                );
            }

            const boost::span<const int> types = rows.types();
            for (const geojson::Feature& feature : found->second) {
                geojson::PropertyMap& properties = feature->get_properties();

                for (std::size_t i = 0; i < columns.size(); i++) {
                    // A NULL cell means the table has no value here, not that the value is null;
                    // a cell of any other unconvertible type has none to publish either.
                    if (static_cast<int>(i) == key_index || !is_convertible_type(types[i])) {
                        continue;
                    }

                    const std::string name = prefix + "." + columns[i];
                    const bool inserted = properties.emplace(
                        name, geojson::JSONProperty(name, get_property(rows, columns[i], types[i]))
                    ).second;

                    // Keeping the property already there would hand the model another source's
                    // value under this entry's name.
                    if (!inserted) {
                        throw std::runtime_error(
                            "joining auxiliary attribute table `" + table + "` of " + gpkg_path +
                            " onto catchment `" + key + "` would overwrite its existing property `" +
                            name + "`"
                        );
                    }
                }
            }
        }

        rows.next();
    }

    std::unordered_set<std::string> reported_ids;
    for (const auto& feature : collection) {
        const std::string& id = feature->get_id();
        if (joined_ids.count(id) > 0 || !reported_ids.insert(id).second) {
            continue;
        }

        const std::string message = "catchment `" + id + "` has no row in auxiliary"
                                    " attribute table `" + table + "` of " + gpkg_path;
        if (required) {
            throw std::runtime_error(message);
        }

        logging::warning((message + "\n").c_str());
    }
}
