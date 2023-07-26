#ifndef NGEN_IO_MDFRAME_VISITORS_HPP
#define NGEN_IO_MDFRAME_VISITORS_HPP

#include "mdarray/mdarray.hpp"
#include <boost/variant.hpp>
#include <span.hpp>
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
    auto operator()(mdarray<T>& md_array) noexcept
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
    auto operator()(mdarray<T>& arr) noexcept
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
    auto operator()(mdarray<T>& arr) noexcept
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
    void operator()(mdarray<T>& arr, boost::span<const std::size_t> index, T value)
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
    typename mdarray<T>::value_type operator()(mdarray<T>& arr, const boost::span<const std::size_t> index)
    {
        return arr.at(index);
    }
};

// Apply std::to_string as a visitor
struct to_string_visitor : public boost::static_visitor<std::string>
{
    template<typename T>
    std::string operator()(T& v)
    {
        return std::to_string(v);
    }
};


} // namespace visitors
} // namespace detail
} // namespace io

#endif // NGEN_IO_MDFRAME_VISITORS_HPP
