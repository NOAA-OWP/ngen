#include <geometry/backends/boost/boost_multipoint.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_multipoint::~boost_multipoint() = default;

auto boost_multipoint::get(size_type n) -> pointer
{
    return &data_.at(n);
}

auto boost_multipoint::get(size_type n) const -> const_pointer
{
    return &data_.at(n);
}

void boost_multipoint::set(size_type n, geometry_collection::const_pointer geom)
{
    auto pt = dynamic_cast<multipoint::const_pointer>(geom);
    if (pt == nullptr) {
        // Not a point
        return;
    }

    auto casted = dynamic_cast<const_pointer>(pt);
    if (casted == nullptr) {
        boost_point new_pt{ pt->x(), pt->y() };
        casted = &new_pt;
    }

    data_.at(n) = *casted;
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
