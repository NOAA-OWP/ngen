#include <geometry/backends/boost/boost_multilinestring.hpp>

namespace ngen {
namespace spatial {
namespace backend {

boost_multilinestring::~boost_multilinestring() = default;

auto boost_multilinestring::get(size_type n) -> pointer
{
    return &data_.at(n);
}

auto boost_multilinestring::get(size_type n) const -> const_pointer
{
    return &data_.at(n);
}

void boost_multilinestring::set(size_type n, geometry_collection::const_pointer geom)
{
    auto line = dynamic_cast<multilinestring::const_pointer>(geom);
    if (line == nullptr) {
        // Not a linestring
        return;
    }

    auto casted = dynamic_cast<const_pointer>(line);
    if (casted == nullptr) {
        boost_linestring new_line{};
        new_line.resize(line->size());
        for (size_t i = 0; i < line->size(); i++) {
            new_line.set(i, line->get(i));
        }
        casted = &new_line;
    }

    data_.at(n) = *casted;
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
