#include <geometry/backends/boost/boost_multilinestring.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_multilinestring::boost_multilinestring() = default;

boost_multilinestring::~boost_multilinestring() = default;

auto boost_multilinestring::get(size_type n) -> reference
{
    return data_.at(n);
}

auto boost_multilinestring::get(size_type n) const -> const_reference
{
    return data_.at(n);
}

void boost_multilinestring::set(size_type n, geometry_collection::const_reference geom)
{

    auto casted = dynamic_cast<const_reference>(geom);
    data_.at(n) = casted;
}

void boost_multilinestring::resize(size_type n)
{
    data_.resize(n);
}

auto boost_multilinestring::size() const noexcept -> size_type
{
    return data_.size();
}

} // namespace backend
} // namespace spatial
} // namespace ngen
