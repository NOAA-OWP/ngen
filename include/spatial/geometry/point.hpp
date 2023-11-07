#ifndef NGEN_SPATIAL_GEOMETRY_POINT_HPP
#define NGEN_SPATIAL_GEOMETRY_POINT_HPP

#include "geometry.hpp"

namespace ngen {
namespace spatial {

//! @brief Spatial Point Base Class
//! 
//! Provides a polymorphic interface to backend point types.
struct point : public virtual geometry
{
    using value_type      = double;
    using reference       = value_type&;
    using const_reference = const value_type&;

    ~point() override = default;

    //! @brief Get the X-coordinate
    //! @return Reference to the X-coordinate for this point.
    virtual reference x() noexcept = 0;

    //! @copydoc point::x()
    virtual const_reference x() const noexcept = 0;

    //! @brief Get the Y-coordinate
    //! @return Reference to the Y-coordinate for this point.
    virtual reference y() noexcept = 0;

    //! @copydoc point::y()
    virtual const_reference y() const noexcept = 0;

    geometry_t type() noexcept override
    {
        return geometry_t::point;
    }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_POINT_HPP
