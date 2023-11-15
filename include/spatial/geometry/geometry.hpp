#ifndef NGEN_SPATIAL_GEOMETRY_GEOMETRY_HPP
#define NGEN_SPATIAL_GEOMETRY_GEOMETRY_HPP

#include <cstddef>
#include <memory>

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

//! @brief Spatial Geometry Base Class
//!
//! Provides a polymorphic interface to geometry types.
struct geometry : public std::enable_shared_from_this<geometry>
{
    using size_type     = std::size_t;

    virtual ~geometry() = default;

    //! @brief Get the type of this geometry.
    //! @return Geometry type enum
    virtual geometry_t type() noexcept { return geometry_t::geometry; };
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_GEOMETRY_HPP
