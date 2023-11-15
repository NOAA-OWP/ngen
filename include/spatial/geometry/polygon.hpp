#ifndef NGEN_SPATIAL_GEOMETRY_POLYGON_HPP
#define NGEN_SPATIAL_GEOMETRY_POLYGON_HPP

#include "geometry.hpp"
#include "point.hpp"
#include "linestring.hpp"

#include "span.hpp"

namespace ngen {
namespace spatial {

//! @brief Spatial Polygon Base Class
//!
//! Provides a polymorphic interface to backend polygon types.
struct polygon : public virtual geometry
{
    using size_type     = std::size_t;
    using pointer       = linestring*;
    using const_pointer = const linestring*;

    ~polygon() override = default;

    //! @brief Get the polygon's outer ring.
    //! @return pointer to a polymorphic linestring (linear ring),
    //!         or `nullptr` if empty.
    virtual pointer outer() noexcept = 0;

    //! @copydoc polygon::outer()
    virtual const_pointer outer() const noexcept = 0;

    //! @brief Get a polygon's inner ring.
    //! @param n Index of inner ring to retrieve.
    //! @return pointer to a polymorphic linestring (linear ring),
    //!         or `nullptr` if no inner ring exists at `n`.
    virtual pointer inner(size_type n) = 0;

    //! @copydoc polygon::inner(size_type)
    virtual const_pointer inner(size_type n) const = 0;

    //! @brief Get the number of rings in this polygon (outer + inners).
    //! @return total number of rings.
    virtual size_type size() const noexcept = 0;

    geometry_t        type() noexcept override { return geometry_t::polygon; }
};
} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_POLYGON_HPP
