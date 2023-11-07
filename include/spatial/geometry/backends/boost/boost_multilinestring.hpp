#ifndef NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_MULTILINESTRING_HPP
#define NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_MULTILINESTRING_HPP

#include "geometry/backends/boost/boost_linestring.hpp"
#include "geometry/multilinestring.hpp"

namespace ngen {
namespace spatial {
namespace backend {

struct boost_multilinestring final : public multilinestring
{
    using size_type     = multilinestring::size_type;
    using pointer       = boost_linestring*;
    using const_pointer = const boost_linestring*;

    ~boost_multilinestring() override;

    pointer get(size_type n) override;
    const_pointer get(size_type n) const override;
    void set(size_type n, geometry_collection::const_pointer geom) override;
    size_type size() const noexcept override;

  private:
    std::vector<boost_linestring> data_;
};

} // namespace backend
} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_GEOMETRY_BACKENDS_BOOST_BOOST_MULTILINESTRING_HPP
