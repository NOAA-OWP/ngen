#ifndef NGEN_SPATIAL_GEOMETRY_MULTIPOINT_HPP
#define NGEN_SPATIAL_GEOMETRY_MULTIPOINT_HPP

#include "geometry_collection.hpp"
#include "point.hpp"

namespace ngen {
namespace spatial {

//! @brief Spatial MultiPoint Base Class
//!
//! Provides a polymorphic interface to backend multipoint types.
struct multipoint : public virtual geometry_collection
{
    using size_type     = geometry_collection::size_type;
    using pointer       = point*;
    using const_pointer = const point*;

    ~multipoint() override = default;

    //! @brief Get the Nth point in this collection.
    //! @param n Index of element to retrieve.
    //! @return Pointer to a point element.
    pointer get(size_type n) override = 0;

    //! @copydoc multipoint::get(size_type);
    const_pointer get(size_type n) const override = 0;

    //! @brief Set the Nth point in this collection.
    //! @param n Index of element to set.
    //! @param geom Geometry object to set.
    void set(size_type n, geometry_collection::const_pointer geom) override = 0;

    geometry_t type() noexcept override
    {
        return geometry_t::multipoint;
    }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_MULTIPOINT_HPP
