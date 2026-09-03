#ifndef NGEN_GEOJSON_HYDROFABRIC_READER_H
#define NGEN_GEOJSON_HYDROFABRIC_READER_H

#include <string>
#include <vector>

#include "HydrofabricReader.hpp"

namespace ngen {
namespace hydrofabric {

/**
 * Reads a hydrofabric stored as a pair of GeoJSON files.
 *
 * This is the original hydrofabric format, and the reason the drivers take two data paths: the
 * divides and the nexuses were always separate files. There is no schema version to detect and
 * nothing to translate -- GeoJSON hydrofabrics carry "id" and "toid" as ordinary feature properties
 * -- so this reader is a thin pass-through to geojson::read().
 *
 * That last point is an assumption, not a check: this reader translates nothing, so it reads only
 * the "id"/"toid" vocabulary that HydrofabricVersion::V1_GEOJSON names. GeoJSON exported from a
 * later hydrofabric would carry that release's column names instead -- `divide_id`, `nexus_id`,
 * `flowpath_toid` -- and would load with empty feature ids rather than being refused. Nothing in
 * ngen produces such files today; supporting them would mean identifying a release from GeoJSON
 * content, which no caller currently needs.
 */
class GeoJSONHydrofabricReader : public HydrofabricReader
{
  public:
    /**
     * @param[in] divides_path Path to the GeoJSON file holding the divides
     * @param[in] nexus_path Path to the GeoJSON file holding the nexuses; may name the same file
     */
    GeoJSONHydrofabricReader(std::string divides_path, std::string nexus_path);

    geojson::GeoJSON read_divides(const std::vector<std::string>& ids = {}) override;

    geojson::GeoJSON read_nexus(const std::vector<std::string>& ids = {}) override;

  private:
    const std::string divides_path_;
    const std::string nexus_path_;
};

} // namespace hydrofabric
} // namespace ngen

#endif // NGEN_GEOJSON_HYDROFABRIC_READER_H
