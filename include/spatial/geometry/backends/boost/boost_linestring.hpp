#ifndef NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_LINESTRING_HPP
#define NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_LINESTRING_HPP

#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>

#include <geometry/linestring.hpp>
#include <geometry/backends/boost/boost_point.hpp>

namespace ngen {
namespace spatial {
namespace backend {

struct boost_linestring final : public linestring
{
    using size_type       = linestring::size_type;
    using pointer         = boost_point*;
    using const_pointer   = const boost_point*;
    using reference       = boost_point&;
    using const_reference = const boost_point&;

    boost_linestring();

    boost_linestring(std::initializer_list<boost_point> pts);

    ~boost_linestring() override;

    size_type       size() const noexcept override;
    reference       get(size_type n) override;
    const_reference get(size_type n) const override;
    void            set(size_type n, const ngen::spatial::point& pt) override;
    void            resize(size_type n) override;
    reference       front() noexcept override;
    reference       back() noexcept override;
    const_reference front() const noexcept override;
    const_reference back() const noexcept override;
    void            swap(linestring& other) noexcept override;

  private:
    ::boost::geometry::model::linestring<boost_point> data_;
};

} // namespace backend
} // namespace spatial
} // namespace ngen

BOOST_GEOMETRY_REGISTER_LINESTRING(ngen::spatial::backend::boost_linestring)

#endif // NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_LINESTRING_HPP
