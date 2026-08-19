#include "geopackage.hpp"
#include "JSONProperty.hpp"

#include <iostream>
#include <unordered_map>

namespace {

// A table can only be named by interpolating it into the statement; quoting keeps that statement
// well-formed for the punctuation and spaces table names are free to carry.
std::string quote_identifier(const std::string& identifier)
{
    std::string quoted = "\"";
    for (const char character : identifier) {
        if (character == '"') {
            quoted += '"';
        }
        quoted += character;
    }
    return quoted + "\"";
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
    ngen::sqlite::database db{gpkg_path};

    // An absent table or key column is a typo rather than a data gap, so `required` does not enter into it.
    if (!db.contains(table)) {
        throw std::runtime_error(
            "auxiliary attribute table `" + table + "` does not exist in " + gpkg_path
        );
    }

    auto rows = db.query("SELECT * FROM " + quote_identifier(table));
    const int key_index = rows.find(key_column);
    if (key_index < 0) {
        throw std::runtime_error(
            "auxiliary attribute table `" + table + "` in " + gpkg_path +
            " has no key column `" + key_column + "`"
        );
    }

    std::unordered_map<std::string, geojson::Feature> unjoined;
    for (const auto& feature : collection) {
        unjoined.emplace(feature->get_id(), feature);
    }

    const auto columns = rows.columns();
    rows.next();
    while (!rows.done()) {
        const auto found = unjoined.find(rows.get<std::string>(key_index));
        if (found != unjoined.end()) {
            geojson::PropertyMap& properties = found->second->get_properties();
            const auto types = rows.types();

            for (std::size_t i = 0; i < columns.size(); i++) {
                // A NULL cell means the table has no value here, not that the value is null.
                if (static_cast<int>(i) == key_index || types[i] == SQLITE_NULL) {
                    continue;
                }

                const std::string name = prefix + "." + columns[i];
                properties.emplace(name, geojson::JSONProperty(name, get_property(rows, columns[i], types[i])));
            }

            unjoined.erase(found);
        }

        rows.next();
    }

    for (const auto& feature : collection) {
        if (unjoined.count(feature->get_id()) == 0) {
            continue;
        }

        const std::string message = "catchment `" + feature->get_id() + "` has no row in auxiliary"
                                    " attribute table `" + table + "` of " + gpkg_path;
        if (required) {
            throw std::runtime_error(message);
        }

        std::cerr << "WARN: " << message << std::endl;
    }
}
