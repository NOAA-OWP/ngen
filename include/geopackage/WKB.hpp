#ifndef NGEN_GEOPACKAGE_GEOMETRY_H
#define NGEN_GEOPACKAGE_GEOMETRY_H

#include <cstdint>
#include <vector>
#include "JSONGeometry.hpp"

namespace geopackage {

// struct geometry_blob
// {
//     struct {
//         uint8_t version;
//         struct {
//             unsigned reserved  : 2;
//             unsigned type      : 1;
//             unsigned empty     : 1;
//             unsigned indicator : 3;
//             unsigned order     : 1;
//         } flags;
//         int32_t srs_id;
//         std::vector<double> envelope;
//     } header;
//     wkb geometry;
// };

namespace wkb {

static inline uint32_t bswap_32(uint32_t x)
{
    return (((x & 0xFF) << 24) |
            ((x & 0xFF00) << 8) |
            ((x & 0xFF0000) >> 8) |
            ((x & 0xFF000000) >> 24));
}

static inline uint64_t bswap_64(uint64_t x)
{
    return (((x & 0xFFULL) << 56) |
            ((x & 0xFF00ULL) << 40) |
            ((x & 0xFF0000ULL) << 24) |
            ((x & 0xFF000000ULL) << 8) |
            ((x & 0xFF00000000ULL) >> 8) |
            ((x & 0xFF0000000000ULL) >> 24) |
            ((x & 0xFF000000000000ULL) >> 40) |
            ((x & 0xFF00000000000000ULL) >> 56));
}

enum wkbGeometryType {
    wkbPoint = 1,
    wkbLineString = 2,
    wkbPolygon = 3,
    wkbMultiPoint = 4,
    wkbMultiLineString = 5,
    wkbMultiPolygon = 6,
    wkbGeometryCollection = 7
};

} // namespace wkb
} // namespace geopackage

#endif // NGEN_GEOPACKAGE_GEOMETRY_H