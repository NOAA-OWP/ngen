#ifndef NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_POLYGON_HPP
#define NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_POLYGON_HPP

#include <boost/geometry/geometries/polygon.hpp>

#include <geometry/polygon.hpp>
#include <geometry/backends/boost/boost_point.hpp>

namespace ngen {
namespace spatial {
namespace boost {

struct boost_polygon : public polygon
{
    using size_type = polygon::size_type;
    
    ~boost_polygon() override = default;

  private:
    ::boost::geometry::model::polygon<boost_point> data_;
};

} // namespace boost
} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_POLYGON_HPP
