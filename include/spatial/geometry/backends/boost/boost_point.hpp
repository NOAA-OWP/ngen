#ifndef NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_POINT_HPP
#define NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_POINT_HPP

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/core/cs.hpp>

#include <geometry/point.hpp>

namespace ngen {
namespace spatial {
namespace boost {

struct boost_point : public point
{
    using value_type      = point::value_type;
    using reference       = point::reference;
    using const_reference = point::const_reference;

    boost_point() = default;

    boost_point(value_type x, value_type y);

    ~boost_point() override;

    reference x() noexcept override;
    reference y() noexcept override;
    const_reference x() const noexcept override;
    const_reference y() const noexcept override;

  private:
    value_type x_;
    value_type y_;
};

} // namespace boost
} // namespace spatial
} // namespace ngen

BOOST_GEOMETRY_REGISTER_POINT_2D(
  ngen::spatial::boost::boost_point,
  double,
  boost::geometry::cs::cartesian,
  x(), y()
)

#endif // NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_POINT_HPP
