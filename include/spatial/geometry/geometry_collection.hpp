#ifndef NGEN_SPATIAL_GEOMETRY_GEOMETRY_COLLECTION_HPP
#define NGEN_SPATIAL_GEOMETRY_GEOMETRY_COLLECTION_HPP

#include "geometry.hpp"

namespace ngen {
namespace spatial {

struct geometry_collection : public virtual geometry
{
    using size_type = geometry::size_type;
    using pointer   = geometry*;

    ~geometry_collection() override = default;

    virtual pointer at(size_type n);

    geometry_t type() noexcept override
    {
        return geometry_t::geometry_collection;
    }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_GEOMETRY_COLLECTION_HPP
