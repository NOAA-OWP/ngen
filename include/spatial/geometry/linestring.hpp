#ifndef NGEN_SPATIAL_GEOMETRY_LINESTRING_HPP
#define NGEN_SPATIAL_GEOMETRY_LINESTRING_HPP

#include "geometry.hpp"
#include "point.hpp"

namespace ngen {
namespace spatial {

//! @brief Spatial LineString Base Class
//!
//! Provides a polymorphic interface to backend linestring types.
struct linestring : public virtual geometry
{
    using size_type     = geometry::size_type;
    using pointer       = point*;
    using const_pointer = const point*;

    ~linestring() override = default;

    //! @brief Get the Nth point in this linestring.
    //! @param n Index of point to retrieve.
    //! @return pointer to a polymorphic point, or `nullptr` if out of bounds.
    virtual pointer at(size_type n) = 0;

    //! @brief Get the starting point for this linestring.
    //! @return pointer to a polymorphic point, or `nullptr` if empty.
    virtual pointer front() noexcept = 0;

    //! @brief Get the ending point for this linestring.
    //! @return pointer to a polymorphic point, or `nullptr` if empty.
    virtual pointer back() noexcept = 0;

    //! @copydoc linestring::at(size_type)
    virtual const_pointer at(size_type n) const  = 0;

    //! @copydoc linestring::front()
    virtual const_pointer front() const noexcept = 0;

    //! @copydoc linestring::back()
    virtual const_pointer back() const noexcept  = 0;

    geometry_t type() noexcept override
    {
        return geometry_t::linestring;
    }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_LINESTRING_HPP
