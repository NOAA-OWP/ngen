#ifndef NGEN_SPATIAL_GEOMETRY_MULTILINESTRING_HPP
#define NGEN_SPATIAL_GEOMETRY_MULTILINESTRING_HPP

#include "geometry_collection.hpp"
#include "linestring.hpp"

namespace ngen {
namespace spatial {

struct multilinestring : public virtual geometry_collection
{
    using size_type     = geometry_collection::size_type;
    using pointer       = linestring*;
    using const_pointer = const linestring*;

    ~multilinestring() override = default;

    pointer       get(size_type n) override = 0;
    const_pointer get(size_type n) const override = 0;
    void          set(size_type n, geometry_collection::const_pointer geom) override = 0;

    geometry_t type() noexcept override
    {
        return geometry_t::multilinestring;
    }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_MULTILINESTRING_HPP
