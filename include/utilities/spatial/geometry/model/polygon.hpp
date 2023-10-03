#ifndef NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_POLYGON_HPP
#define NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_POLYGON_HPP

#include "../types.hpp"
#include "../backend.hpp"
#include "../../../span.hpp"

namespace ngen {
namespace spatial {

template<typename BackendPolicy>
struct polygon
{
    using backend         = ngen::spatial::backend<BackendPolicy>;
    using value_type      = typename backend::value_type;
    using element_type    = typename backend::linestring_type;
    using size_type       = typename backend::size_type;
    using reference       = element_type&;
    using const_reference = const element_type&;

    polygon(boost::span<element_type> rings)
      : data_(backend::make_polygon(rings)){};

    reference outer() noexcept
    {
        return backend::get_ring(data_, 0);
    }

    reference at(size_type i)
    {
        return backend::get_ring(data_, i);
    }

    size_type size() const noexcept
    {
        return backend::num_rings(data_);
    }

  private:
    typename backend::polygon_type data_;
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_POLYGON_HPP
