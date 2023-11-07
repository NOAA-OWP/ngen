#ifndef NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_LINEARRING_HPP
#define NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_LINEARRING_HPP

#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/register/ring.hpp>

#include <geometry/linestring.hpp>
#include <geometry/backends/boost/boost_point.hpp>

namespace ngen {
namespace spatial {
namespace backend {

struct boost_linearring final : public linestring
{
    using size_type     = linestring::size_type;
    using pointer       = boost_point*;
    using const_pointer = const boost_point*;

    boost_linearring(std::initializer_list<boost_point> pts);

    boost_linearring(::boost::geometry::model::ring<boost_point>& ring);

    ~boost_linearring() override = default;

    size_type     size()                      const noexcept override;
    pointer       get(size_type n)                           override;
    void          set(size_type n, ngen::spatial::point* pt) override;
    void          resize(size_type n)                        override;
    pointer       front()                           noexcept override;
    pointer       back()                            noexcept override;
    const_pointer front()                     const noexcept override;
    const_pointer back()                      const noexcept override;

  private:
    ::boost::geometry::model::ring<boost_point> data_;
};

} // namespace backend
} // namespace spatial
} // namespace ngen

BOOST_GEOMETRY_REGISTER_RING(ngen::spatial::backend::boost_linearring)

#endif // NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_LINEARRING_HPP
