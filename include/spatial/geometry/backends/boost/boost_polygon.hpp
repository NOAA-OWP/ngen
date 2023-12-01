#ifndef NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_POLYGON_HPP
#define NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_POLYGON_HPP

#include <boost/geometry/geometries/polygon.hpp>

#include <geometry/polygon.hpp>
#include <geometry/backends/boost/boost_point.hpp>
#include "geometry/backends/boost/boost_linearring.hpp"

namespace ngen {
namespace spatial {
namespace backend {

struct boost_polygon final : public polygon
{
    using size_type           = polygon::size_type;
    using pointer             = boost_linearring*;
    using const_pointer       = const boost_linearring*;
    using reference           = boost_linearring&;
    using const_reference     = const boost_linearring&;

    boost_polygon();

    ~boost_polygon() override;

    reference       outer() noexcept override;
    const_reference outer() const noexcept override;
    reference       inner(size_type n) override;
    const_reference inner(size_type n) const override;
    size_type     size() const noexcept override;

  private:
    boost_linearring              outer_;
    std::vector<boost_linearring> inner_;
};

} // namespace backend
} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_POLYGON_HPP
