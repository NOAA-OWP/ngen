#include "GeoPackage.hpp"

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
        std::inserter(property_types, property_types.end()), [](const std::string& name, int type) {
            return std::make_pair(name, type);
        }
    );

    for (auto& col : property_types) {
        const auto name = col.first;
        const auto type = col.second;
        if (name == geom_col) {
            continue;
        }

        geojson::JSONProperty* property = nullptr;
        switch(type) {
            case SQLITE_INTEGER:
                *property = geojson::JSONProperty(name, row.get<int>(name));
                break;
            case SQLITE_FLOAT:
                *property = geojson::JSONProperty(name, row.get<double>(name));
                break;
            case SQLITE_TEXT:
                *property = geojson::JSONProperty(name, row.get<std::string>(name));
                break;
            default:
                *property = geojson::JSONProperty(name, "null");
                break;
        }

        properties.emplace(col, std::move(*property));
    }

    return properties;
}