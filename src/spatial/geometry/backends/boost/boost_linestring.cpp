#include <geometry/backends/boost/boost_linestring.hpp>

namespace ngen {
namespace spatial {
namespace boost {

auto boost_linestring::at(size_type n) -> pointer
{
    return &data_.at(n);
}

auto boost_linestring::front() noexcept -> pointer
{
    return &data_.front();
}

auto boost_linestring::back() noexcept -> pointer
{
    return &data_.back();
}

auto boost_linestring::at(size_type n) const -> const_pointer
{
    return &data_.at(n);
}

auto boost_linestring::front() const noexcept -> const_pointer
{
    return &data_.front();
}

auto boost_linestring::back() const noexcept -> const_pointer
{
    return &data_.back();
}


} // namespace boost
} // namespace spatial
} // namespace ngen
