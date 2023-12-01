#include <geometry/backends/boost/boost_polygon.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_polygon::boost_polygon() = default;

boost_polygon::~boost_polygon() = default;

auto boost_polygon::outer() noexcept -> reference
{
    return outer_;
}

auto boost_polygon::outer() const noexcept -> const_reference
{
    return outer_;
}

auto boost_polygon::inner(size_type n) -> reference
{
    return inner_.at(n);
}

auto boost_polygon::inner(size_type n) const -> const_reference
{
    return inner_.at(n);
}

auto boost_polygon::size() const noexcept -> size_type
{
    return 1 + inner_.size(); // 1 for outer_
}

} // namespace backend
} // namespace spatial
} // namespace ngen
