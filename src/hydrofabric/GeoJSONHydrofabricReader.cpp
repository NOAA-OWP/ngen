#include "GeoJSONHydrofabricReader.hpp"

#include <utility>

namespace ngen {
namespace hydrofabric {

GeoJSONHydrofabricReader::GeoJSONHydrofabricReader(std::string divides_path, std::string nexus_path)
  : divides_path_(std::move(divides_path))
  , nexus_path_(std::move(nexus_path))
{}

geojson::GeoJSON GeoJSONHydrofabricReader::read_divides(const std::vector<std::string>& ids)
{
    return geojson::read(divides_path_, ids);
}

geojson::GeoJSON GeoJSONHydrofabricReader::read_nexus(const std::vector<std::string>& ids)
{
    return geojson::read(nexus_path_, ids);
}

} // namespace hydrofabric
} // namespace ngen
