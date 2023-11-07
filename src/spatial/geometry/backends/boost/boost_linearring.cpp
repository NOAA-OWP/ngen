#include <geometry/backends/boost/boost_linearring.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_linearring::boost_linearring(std::initializer_list<boost_point> pts)
  : data_(pts){};

boost_linearring::boost_linearring(::boost::geometry::model::ring<boost_point>& ring)
  : data_(ring){};

auto boost_linearring::size() const noexcept -> size_type
{
    return data_.size();
}

auto boost_linearring::get(size_type n) -> pointer
{
    return &data_.at(n);
}

void boost_linearring::set(size_type n, ngen::spatial::point* pt)
{
    // TODO: This might be expensive
    decltype(auto) casted = dynamic_cast<boost_point*>(pt);
    if (casted == nullptr) {
        // If `pt` is not a boost_point, then
        // we need to copy from the backend into
        // this backend.
        boost_point new_pt {pt->x(), pt->y()};
        casted = &new_pt;
    }

    data_.at(n) = *casted;
}

void boost_linearring::resize(size_type n)
{
    data_.resize(n);
}

auto boost_linearring::front() noexcept -> pointer
{
    return &data_.front();
}

auto boost_linearring::back() noexcept -> pointer
{
    return &data_.back();
}

auto boost_linearring::front() const noexcept -> const_pointer
{
    return &data_.front();
}

auto boost_linearring::back() const noexcept -> const_pointer
{
    return &data_.back();
}

} // namespace backend
} // namespace spatial
} // namespace ngen
