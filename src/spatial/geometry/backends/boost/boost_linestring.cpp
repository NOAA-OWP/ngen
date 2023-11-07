#include <geometry/backends/boost/boost_linestring.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_linestring::boost_linestring(std::initializer_list<boost_point> pts)
  : data_(pts){};

auto boost_linestring::size() const noexcept -> size_type
{
    return data_.size();
}

auto boost_linestring::get(size_type n) -> pointer
{
    return &data_.at(n);
}

void boost_linestring::set(size_type n, ngen::spatial::point* pt)
{
    // TODO: This might be expensive
    auto casted = dynamic_cast<boost_point*>(pt);
    if (casted == nullptr) {
        // If `pt` is not a boost_point, then
        // we need to copy from the backend into
        // this backend.
        boost_point new_pt {pt->x(), pt->y()};
        casted = &new_pt;
    }

    data_.at(n) = *casted;
}

void boost_linestring::resize(size_type n)
{
    data_.resize(n);
}

auto boost_linestring::front() noexcept -> pointer
{
    return &data_.front();
}

auto boost_linestring::back() noexcept -> pointer
{
    return &data_.back();
}

auto boost_linestring::front() const noexcept -> const_pointer
{
    return &data_.front();
}

auto boost_linestring::back() const noexcept -> const_pointer
{
    return &data_.back();
}

} // namespace backend
} // namespace spatial
} // namespace ngen
