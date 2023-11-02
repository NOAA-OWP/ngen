#ifndef NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_LINESTRING_HPP
#define NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_LINESTRING_HPP

#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>

#include <geometry/linestring.hpp>
#include <geometry/backends/boost/boost_point.hpp>

namespace ngen {
namespace spatial {
namespace boost {

struct boost_linestring : public linestring
{
    using size_type     = linestring::size_type;
    using pointer       = boost_point*;
    using const_pointer = const boost_point*;

    boost_linestring(std::initializer_list<boost_point> pts)
      : data_(pts){};

    ~boost_linestring() override = default;

    size_type size() const noexcept override;
    pointer get(size_type n) override;
    void    set(size_type n,  ngen::spatial::point* pt) override;
    pointer front() noexcept override;
    pointer back() noexcept override;
    const_pointer front() const noexcept override;
    const_pointer back() const noexcept override;

  private:
    ::boost::geometry::model::linestring<boost_point> data_;
};

} // namespace boost
} // namespace spatial
} // namespace ngen

BOOST_GEOMETRY_REGISTER_LINESTRING(ngen::spatial::boost::boost_linestring)

#endif // NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_LINESTRING_HPP
