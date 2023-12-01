#ifndef NGEN_SPATIAL_GEOMETRY_MULTIPOLYGON_HPP
#define NGEN_SPATIAL_GEOMETRY_MULTIPOLYGON_HPP

#include "geometry_collection.hpp"
#include "polygon.hpp"

namespace ngen {
namespace spatial {

//! @brief Spatial MultiPolygon Base Class
//!
//! Provides a polymorphic interface to backend multipolygon types.
struct multipolygon : public virtual geometry_collection
{
    using size_type          = geometry_collection::size_type;
    using pointer            = polygon*;
    using const_pointer      = const polygon*;
    using reference          = polygon&;
    using const_reference    = const polygon&;

    ~multipolygon() override = default;

    //! @brief Get the Nth polygon in this collection.
    //! @param n Index of element to retrieve.
    //! @return Pointer to a polygon element.
    reference get(size_type n) override = 0;

    //! @copydoc multipolygon::get(size_type);
    const_reference get(size_type n) const override = 0;

    //! @brief Set the Nth polygon in this collection.
    //! @param n Index of element to set.
    //! @param geom Geometry object to set.
    void       set(size_type n, geometry_collection::const_reference geom) override = 0;

    geometry_t type() noexcept override { return geometry_t::multipolygon; }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_MULTIPOLYGON_HPP
