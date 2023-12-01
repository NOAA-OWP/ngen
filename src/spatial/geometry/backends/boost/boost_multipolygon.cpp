#include <geometry/backends/boost/boost_multipolygon.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_multipolygon::boost_multipolygon() = default;

boost_multipolygon::~boost_multipolygon() = default;

auto boost_multipolygon::get(size_type n) -> reference
{
    return data_.at(n);
}

auto boost_multipolygon::get(size_type n) const -> const_reference
{
    return data_.at(n);
}

void boost_multipolygon::set(size_type n, geometry_collection::const_reference geom)
{
    auto casted = dynamic_cast<const_reference>(geom);
    data_.at(n) = casted;
}

void boost_multipolygon::resize(size_type n)
{
    data_.resize(n);
}

auto boost_multipolygon::size() const noexcept -> size_type
{
    return data_.size();
}

} // namespace backend
} // namespace spatial
} // namespace ngen
