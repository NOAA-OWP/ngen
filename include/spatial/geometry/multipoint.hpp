#ifndef NGEN_SPATIAL_GEOMETRY_MULTIPOINT_HPP
#define NGEN_SPATIAL_GEOMETRY_MULTIPOINT_HPP

#include "geometry_collection.hpp"
#include "point.hpp"

namespace ngen {
namespace spatial {

struct multipoint : public virtual geometry_collection
{
    using size_type     = geometry_collection::size_type;
    using pointer       = point*;
    using const_pointer = const point*;

    ~multipoint() override = default;

    pointer       get(size_type n) override = 0;
    const_pointer get(size_type n) const override = 0;
    void          set(size_type n, geometry_collection::const_pointer geom) override = 0;

    geometry_t type() noexcept override
    {
        return geometry_t::multipoint;
    }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_MULTIPOINT_HPP
