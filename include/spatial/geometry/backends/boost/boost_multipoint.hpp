#ifndef NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_MULTIPOINT_HPP
#define NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_MULTIPOINT_HPP

#include <vector>

#include "geometry/backends/boost/boost_point.hpp"
#include "geometry/multipoint.hpp"

namespace ngen {
namespace spatial {
namespace backend {

struct boost_multipoint final : public multipoint
{
    using size_type     = multipoint::size_type;
    using pointer       = boost_point*;
    using const_pointer = const boost_point*;

    ~boost_multipoint() override;

    pointer get(size_type n) override;
    const_pointer get(size_type n) const override;
    void set(size_type n, geometry_collection::const_pointer geom) override;
    size_type size() const noexcept override;

  private:
    std::vector<boost_point> data_;
};

} // namespace backend
} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_MULTIPOINT_HPP
