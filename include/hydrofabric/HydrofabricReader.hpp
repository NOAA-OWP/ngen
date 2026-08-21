#ifndef NGEN_HYDROFABRIC_READER_H
#define NGEN_HYDROFABRIC_READER_H

#include <string>
#include <vector>

#include "FeatureBuilder.hpp"

namespace ngen {
namespace hydrofabric {

/**
 * Reads the catchment and nexus features of a hydrofabric, whatever format it is stored in.
 *
 * A hydrofabric is a pair of roles -- the divides (catchments) and the nexuses they drain to -- and
 * those roles may live in one file or two. Implementations own whatever handles that takes and
 * present the pair here, so a caller loading a hydrofabric neither opens files nor knows which
 * format it was handed.
 *
 * The interface deliberately names roles rather than layers. A GeoPackage hydrofabric keeps its
 * roles in named tables, but a GeoJSON hydrofabric has no layers at all -- its roles are two whole
 * files -- so a layer-keyed interface could only be honored by one of the two. Reading some other
 * table of a GeoPackage is a different job, served directly by ngen::geopackage::GeoPackageReader.
 */
class HydrofabricReader
{
  public:
    virtual ~HydrofabricReader() = default;

    /**
     * Read the hydrofabric's divides.
     *
     * @param[in] ids Optional subset of divide IDs to capture; if empty, all divides are read. IDs
     *            that the hydrofabric does not contain are silently ignored, so the result is the
     *            intersection of the hydrofabric and @p ids.
     * @return geojson::GeoJSON Collection of divide features
     * @throws std::runtime_error if the divides cannot be read
     */
    virtual geojson::GeoJSON read_divides(const std::vector<std::string>& ids = {}) = 0;

    /**
     * Read the hydrofabric's nexuses.
     *
     * @param[in] ids Optional subset of nexus IDs to capture; if empty, all nexuses are read. IDs
     *            that the hydrofabric does not contain are silently ignored.
     * @return geojson::GeoJSON Collection of nexus features
     * @throws std::runtime_error if the nexuses cannot be read
     */
    virtual geojson::GeoJSON read_nexus(const std::vector<std::string>& ids = {}) = 0;
};

} // namespace hydrofabric
} // namespace ngen

#endif // NGEN_HYDROFABRIC_READER_H
