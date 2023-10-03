#ifndef NGEN_UTILITIES_SPATIAL_GEOMETRY_TYPES_HPP
#define NGEN_UTILITIES_SPATIAL_GEOMETRY_TYPES_HPP
namespace ngen {
namespace spatial {

enum class geometry_type
{
    point,
    linestring,
    polygon,
    multipoint,
    multilinestring,
    multipolygon,
    geometry_collection
};

template<typename BackendPolicy>
struct point;

template<typename BackendPolicy>
struct linestring;

template<typename BackendPolicy>
struct polygon;

template<typename BackendPolicy>
struct geometry;

} // namespace spatial
} // namespace ngen
#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_TYPES_HPP
