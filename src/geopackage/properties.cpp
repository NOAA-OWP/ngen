#include "GeoPackage.hpp"
#include "JSONProperty.hpp"

geojson::JSONProperty get_property(const geopackage::sqlite_iter& row, const std::string& name, int type)
{
    if (type == SQLITE_INTEGER) {
        auto val = row.get<int>(name);
        return geojson::JSONProperty(name, val);
    } else if (type == SQLITE_FLOAT) {
        auto val = row.get<double>(name);
        return geojson::JSONProperty(name, val);
    } else if (type == SQLITE_TEXT) {
        auto val = row.get<std::string>(name);
        return geojson::JSONProperty(name, val);
    } else {
        return geojson::JSONProperty(name, "null");
    }
}

geojson::PropertyMap geopackage::build_properties(
    const sqlite_iter& row,
    const std::string& geom_col
)
{
    geojson::PropertyMap properties;

    std::map<std::string, int> property_types;
    const auto data_cols = row.columns();
    const auto data_types = row.types();

    std::transform(
        data_cols.begin(),
        data_cols.end(),
        data_types.begin(),
        std::inserter(property_types, property_types.end()), [](std::string name, int type) {
            return std::make_pair(name, type);
        }
    );

    for (auto& col : property_types) {
        auto name = col.first;
        const auto type = col.second;
        if (name == geom_col) {
            continue;
        }

        geojson::JSONProperty property = get_property(row, name, type);
        properties.emplace(name, std::move(property));
    }

    return properties;
}
