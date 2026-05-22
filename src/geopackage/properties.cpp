#include "geopackage.hpp"
#include "JSONProperty.hpp"

geojson::JSONProperty get_property(
    const ngen::sqlite::database::iterator& row,
    const std::string& name,
    int type
)
{
    switch(type) {
        case SQLITE_INTEGER:
            return { name, row.get<int>(name)};
        case SQLITE_FLOAT:
            return { name, row.get<double>(name)};
        case SQLITE_TEXT:
            return { name, row.get<std::string>(name)};
        default:
            return { name, "null" };
    }
}

geojson::PropertyMap ngen::geopackage::build_properties(
    const ngen::sqlite::database::iterator& row,
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
