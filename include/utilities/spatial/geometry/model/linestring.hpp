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
    using size_type       = typename backend::size_type;
    using element_type    = ngen::spatial::point<BackendPolicy>;
    using reference       = typename element_type::proxy;
    using const_reference = const typename element_type::proxy;

    struct proxy;

    linestring(boost::span<element_type> points)
      : data_(backend::make_linestring(points)){};

    reference start_point() noexcept
    {
        return reference{backend::get_point(data_, 0)};
    }

    reference end_point() noexcept
    {
        return reference{backend::get_point(data_, size() - 1)};
    }

    reference at(size_type i)
    {
        return reference{backend::get_point(data_, i)};
    }

    size_type size() const noexcept
    {
        return backend::num_points(data_);
    }

  private:
    typename backend::linestring_type data_;
};

template<typename BackendPolicy>
struct linestring<BackendPolicy>::proxy : public linestring<BackendPolicy>
{
    using base_type       = linestring<BackendPolicy>;
    using backend         = typename base_type::backend;

    explicit proxy(const base_type& ref)
      : data_(ref.data_){};

    explicit proxy(const typename backend::linestring_type& ref)
      : data_(ref){};

  private:
    typename backend::linestring_type& data_;
};

template<typename BackendPolicy>
struct ring : public linestring<BackendPolicy>
{
    using base_type       = linestring<BackendPolicy>;
    using backend         = typename base_type::backend;
    using value_type      = typename base_type::value_type;
    using size_type       = typename base_type::size_type;
    using element_type    = typename base_type::element_type;
    using reference       = typename base_type::reference;
    using const_reference = typename base_type::const_reference;

    struct proxy;

    ring(boost::span<element_type> points)
      : data_(backend::make_ring(points)){};

  private:
    typename backend::ring_type data_;
};

template<typename BackendPolicy>
struct ring<BackendPolicy>::proxy : public linestring<BackendPolicy>
{
    using base_type = ring<BackendPolicy>;
    using backend   = typename base_type::backend;

    explicit proxy(const base_type& ref)
      : data_(ref.data_){};

    explicit proxy(const typename backend::ring_type& ref)
      : data_(ref){};

  private:
    typename backend::ring_type& data_;
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_LINESTRING_HPP
