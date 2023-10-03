#ifndef NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_LINESTRING_HPP
#define NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_LINESTRING_HPP

#include "../types.hpp"
#include "../backend.hpp"
#include "../../../span.hpp"

namespace ngen {
namespace spatial {

template<typename BackendPolicy>
struct linestring
{
    using backend         = ngen::spatial::backend<BackendPolicy>;
    using value_type      = typename backend::value_type;
    using element_type    = typename backend::point_type;
    using size_type       = typename backend::size_type;
    using reference       = element_type&;
    using const_reference = const element_type&;

    linestring(boost::span<element_type> points)
      : data_(backend::make_linestring(points)){};

    reference start_point() noexcept
    {
        return backend::get_point(data_, 0);
    }

    reference end_point() noexcept
    {
        return backend::get_point(data_, size() - 1);
    }

    reference at(size_type i)
    {
        return backend::get_point(data_, i);
    }

    size_type size() const noexcept
    {
        return backend::num_points(data_);
    }

  private:
    typename backend::linestring_type data_;
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_LINESTRING_HPP
