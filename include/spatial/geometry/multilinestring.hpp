#ifndef NGEN_SPATIAL_GEOMETRY_MULTILINESTRING_HPP
#define NGEN_SPATIAL_GEOMETRY_MULTILINESTRING_HPP

#include "geometry_collection.hpp"
#include "linestring.hpp"

namespace ngen {
namespace spatial {

//! @brief Spatial MultiLineString Base Class
//!
//! Provides a polymorphic interface to backend multilinestring types.
struct multilinestring : public virtual geometry_collection
{
    using size_type     = geometry_collection::size_type;
    using pointer       = linestring*;
    using const_pointer = const linestring*;

    ~multilinestring() override = default;

    //! @brief Get the Nth linestring in this collection.
    //! @param n Index of element to retrieve.
    //! @return Pointer to a linestring element.
    pointer get(size_type n) override = 0;

    //! @copydoc multilinestring::get(size_type);
    const_pointer get(size_type n) const override = 0;

    //! @brief Set the Nth linestring in this collection.
    //! @param n Index of element to set.
    //! @param geom Geometry object to set.
    void set(size_type n, geometry_collection::const_pointer geom) override = 0;

    geometry_t type() noexcept override
    {
        return geometry_t::multilinestring;
    }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_MULTILINESTRING_HPP
