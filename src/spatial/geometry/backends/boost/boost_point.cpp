#include <geometry/backends/boost/boost_point.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_point::boost_point(value_type x, value_type y)
  : x_(x)
  , y_(y){};

boost_point::~boost_point() = default;

auto boost_point::x() noexcept -> reference
{
    return x_;
}

auto boost_point::y() noexcept -> reference
{
    return y_;
}

auto boost_point::x() const noexcept -> const_reference
{
    return x_;
}

auto boost_point::y() const noexcept -> const_reference
{
    return y_;
}

} // namespace backend
} // namespace spatial
} // namespace ngen
