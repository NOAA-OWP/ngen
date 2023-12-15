#include <geometry/backends/boost/boost_linearring.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_linearring::boost_linearring() = default;

boost_linearring::~boost_linearring() = default;

boost_linearring::boost_linearring(std::initializer_list<boost_point> pts)
  : data_(pts){};

boost_linearring::boost_linearring(
  ::boost::geometry::model::ring<boost_point>& ring
)
  : data_(ring){};

auto boost_linearring::size() const noexcept -> size_type
{
    return data_.size();
}

auto boost_linearring::get(size_type n) -> reference
{
    return data_.at(n);
}

auto boost_linearring::get(size_type n) const -> const_reference
{
    return data_.at(n);
}

void boost_linearring::set(size_type n, const ngen::spatial::point& pt)
{
    decltype(auto) casted = dynamic_cast<const_reference>(pt);
    get(n) = casted;
}

void boost_linearring::resize(size_type n)
{
    data_.resize(n);
}

auto boost_linearring::front() noexcept -> reference
{
    return data_.front();
}

auto boost_linearring::back() noexcept -> reference
{
    return data_.back();
}

auto boost_linearring::front() const noexcept -> const_reference
{
    return data_.front();
}

auto boost_linearring::back() const noexcept -> const_reference
{
    return data_.back();
}

void boost_linearring::swap(linestring& other) noexcept
{
    linestring* ptr = &other;

    auto ring = dynamic_cast<boost_linearring*>(ptr);
    if (ring != nullptr) {
        data_.swap(ring->data_);
        return;
    }
}

void boost_linearring::clone(linestring& other) noexcept
{
    linestring* ptr = &other;

    auto ring = dynamic_cast<boost_linearring*>(ptr);
    if (ring != nullptr) {
        data_ = ring->data_;
    } else {
        const auto size_ = ptr->size();
        data_.resize(ptr->size());
        for (size_type i = 0; i < size_; i++) {
            set(i, ptr->get(i));
        }
    }
}

} // namespace backend
} // namespace spatial
} // namespace ngen
