#include <geometry/backends/boost/boost_multipoint.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_multipoint::boost_multipoint() = default;

boost_multipoint::~boost_multipoint() = default;

auto boost_multipoint::get(size_type n) -> reference
{
    return data_.at(n);
}

auto boost_multipoint::get(size_type n) const -> const_reference
{
    return data_.at(n);
}

void boost_multipoint::set(size_type n, geometry_collection::const_reference geom)
{
    auto casted = dynamic_cast<const_reference>(geom);
    data_.at(n) = casted;
}

void boost_multipoint::resize(size_type n)
{
    data_.resize(n);
}

auto boost_multipoint::size() const noexcept -> size_type
{
    return data_.size();
}

} // namespace backend
} // namespace spatial
} // namespace ngen
