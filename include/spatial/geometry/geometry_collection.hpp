#ifndef NGEN_SPATIAL_GEOMETRY_GEOMETRY_COLLECTION_HPP
#define NGEN_SPATIAL_GEOMETRY_GEOMETRY_COLLECTION_HPP

#include "geometry.hpp"

namespace ngen {
namespace spatial {

//! @brief Spatial Geometry Collection Base Class
//!
//! Provides a polymorphic interface to backend geometry collection types.
struct geometry_collection : public virtual geometry
{
    using size_type     = geometry::size_type;
    using pointer       = geometry*;
    using const_pointer = const geometry*;

    ~geometry_collection() override = default;

    //! @brief Get the Nth geometry in this collection.
    //! @param n Index of element to retrieve.
    //! @return Pointer to a geometry element.
    virtual pointer get(size_type n) = 0;

    //! @copydoc geometry_collection::get(size_type);
    virtual const_pointer get(size_type n) const = 0;

    //! @brief Set the Nth geometry in this collection.
    //! @param n Index of element to set.
    //! @param geom Geometry object to set.
    virtual void set(size_type n, const_pointer geom) = 0;

    //! @brief Set the size of this collection.
    //! @note If `n` is smaller than the current size, the collection is truncated.
    //! @param n Number of geometries to be contained in this collection.
    virtual void resize(size_type n) = 0;

    //! @brief Get the number of geometries in this collection.
    //! @return Number of geometries.
    virtual size_type size() const noexcept = 0;

    geometry_t type() noexcept override
    {
        return geometry_t::geometry_collection;
    }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_GEOMETRY_COLLECTION_HPP
