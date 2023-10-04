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
    using size_type       = typename backend::size_type;
    using element_type    = ngen::spatial::linestring<BackendPolicy>;
    using reference       = typename element_type::proxy;
    using const_reference = const typename element_type::proxy;

    struct proxy;

    polygon(boost::span<element_type> rings)
      : data_(backend::make_polygon(rings)){};

    reference outer() noexcept
    {
        return reference{backend::get_ring(data_, 0)};
    }

    reference at(size_type i)
    {
        return reference{backend::get_ring(data_, i)};
    }

    size_type size() const noexcept
    {
        return backend::num_rings(data_);
    }

  private:
    typename backend::polygon_type data_;
};

template<typename BackendPolicy>
struct polygon<BackendPolicy>::proxy : public polygon<BackendPolicy>
{
    using base_type = polygon<BackendPolicy>;
    using backend   = typename base_type::backend;

    explicit proxy(const base_type& ref)
      : data_(ref.data_){};

    explicit proxy(const typename backend::linestring_type& ref)
      : data_(ref){};

  private:
    typename backend::polygon_type& data_;
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_POLYGON_HPP
