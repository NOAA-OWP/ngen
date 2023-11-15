#ifndef NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_MULTIPOLYGON_HPP
#define NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_MULTIPOLYGON_HPP

#include "geometry/backends/boost/boost_polygon.hpp"
#include "geometry/multipolygon.hpp"

namespace ngen {
namespace spatial {
namespace backend {

struct boost_multipolygon final : public multipolygon
{
    using size_type     = multipolygon::size_type;
    using pointer       = boost_polygon*;
    using const_pointer = const boost_polygon*;

    ~boost_multipolygon() override;

    pointer       get(size_type n) override;
    const_pointer get(size_type n) const override;
    void          set(size_type n, geometry_collection::const_pointer geom) override;
    void          resize(size_type n) override;
    size_type     size() const noexcept override;

  private:
    std::vector<boost_polygon> data_;
};

} // namespace backend
} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_MULTIPOLYGON_HPP
