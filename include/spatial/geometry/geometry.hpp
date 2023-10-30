#ifndef NGEN_SPATIAL_GEOMETRY_GEOMETRY_HPP
#define NGEN_SPATIAL_GEOMETRY_GEOMETRY_HPP

#include <cstddef>
namespace ngen {
namespace spatial {

enum class geometry_t
{
    point,
    linestring,
    polygon,
    multipoint,
    multilinestring,
    multipolygon,
    geometry_collection,
    geometry
};

/**
 * Spatial Geometry Base Class
 * 
 * Provides a polymorphic interface to geometry types.
 */
struct geometry
{
    using size_type = std::size_t;

    virtual ~geometry() = default;

    virtual geometry_t type() noexcept
    {
        return geometry_t::geometry;
    };
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_GEOMETRY_HPP
