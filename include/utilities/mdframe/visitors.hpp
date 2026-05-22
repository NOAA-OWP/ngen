#ifndef NGEN_IO_MDFRAME_VISITORS_HPP
#define NGEN_IO_MDFRAME_VISITORS_HPP

#include "mdarray/mdarray.hpp"
#include <boost/variant.hpp>
#include <boost/core/span.hpp>
#include <traits.hpp>

namespace ngen {
namespace detail {
namespace visitors {

/**
 * mdarray visitor for retrieving the size of the mdarray
 */
struct mdarray_size
    : public boost::static_visitor<std::size_t>
{
    template<typename T>
    auto operator()(const mdarray<T>& md_array) const noexcept
    {
        return md_array.size();
    }
};

/**
 * mdarray visitor for retrieving the rank of the mdarray
 */
struct mdarray_rank
    : public boost::static_visitor<std::size_t>
{
    template<typename T>
    auto operator()(const mdarray<T>& arr) const noexcept
    {
        return arr.rank();
    }
};

/**
 * mdarray visitor for retrieving the shape of the mdarray
 */
struct mdarray_shape
    : public boost::static_visitor<boost::span<const std::size_t>>
{
    template<typename T>
    auto operator()(const mdarray<T>& arr) const noexcept
    {
        return arr.shape();
    }
};

/**
 * mdarray visitor for inserting a value
 */
struct mdarray_insert
    : public boost::static_visitor<void>
{
    template<typename T>
    void operator()(T& arr, boost::span<const std::size_t> index, typename T::value_type value)
    {
        arr.insert(index, value);
    }
};

/**
 * mdarray visitor for indexed access
 */
template<typename... SupportedTypes>
struct mdarray_at
    : public boost::static_visitor<
        typename traits::type_list<SupportedTypes...>::variant_scalar
    >
{
    template<typename T>
    typename T::value_type operator()(const T& arr, const boost::span<const std::size_t> index) const
    {
        return arr.at(index);
    }
};

// Apply std::to_string as a visitor
struct to_string_visitor : public boost::static_visitor<std::string>
{
    template<typename T>
    std::string operator()(const T& v) const
    {
        return std::to_string(v);
    }
};


} // namespace visitors
} // namespace detail
} // namespace ngen

#endif // NGEN_IO_MDFRAME_VISITORS_HPP
