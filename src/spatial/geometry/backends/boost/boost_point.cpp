#include <geometry/backends/boost/boost_point.hpp>

namespace ngen {
namespace spatial {
namespace boost {

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

} // namespace boost
} // namespace spatial
} // namespace ngen
