#include <geometry/backends/boost/boost_multipolygon.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_multipolygon::~boost_multipolygon() = default;

auto boost_multipolygon::get(size_type n) -> pointer
{
    return &data_.at(n);
}

auto boost_multipolygon::get(size_type n) const -> const_pointer
{
    return &data_.at(n);
}

void boost_multipolygon::set(size_type n, geometry_collection::const_pointer geom)
{
    auto poly = dynamic_cast<multipolygon::const_pointer>(geom);
    if (poly == nullptr) {
        // Not a point
        return;
    }

    auto casted = dynamic_cast<const_pointer>(poly);
    if (casted == nullptr) {
        boost_polygon new_polygon{};
        // TODO: new_polygon.outer() = poly->outer();
        for (size_t i = 0; i < poly->size() - 1; i++) {
            // TODO: new_polygon.inner(i) = poly->inner(i);
        }
        casted = &new_polygon;
    }

    data_.at(n) = *casted;
}

void boost_multipolygon::resize(size_type n)
{
    data_.resize(n);
}

auto boost_multipolygon::size() const noexcept -> size_type
{
    return data_.size();
}

} // namespace backend
} // namespace spatial
} // namespace ngen
