#ifndef NGEN_GEOPACKAGE_GEOMETRY_H
#define NGEN_GEOPACKAGE_GEOMETRY_H

#include <boost/geometry/io/io.hpp>
#include <cstdint>
#include <vector>

namespace geopackage {

struct GeoPackageGeometryHeader
{
    uint8_t version;
    struct {
        unsigned reserved  : 2;
        unsigned type      : 1;
        unsigned empty     : 1;
        unsigned indicator : 3;
        unsigned order     : 1;
    } flags;
    int32_t srs_id;
    std::vector<double> envelope;
};

struct GeoPackageGeometry
{
    GeoPackageGeometryHeader header;
    std::vector<uint8_t>     geometry;
};
} // namespace geopackage

#endif // NGEN_GEOPACKAGE_GEOMETRY_H