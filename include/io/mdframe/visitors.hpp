#ifndef NGEN_IO_MDFRAME_VISITORS_HPP
#define NGEN_IO_MDFRAME_VISITORS_HPP

#include <boost/variant.hpp>
#include <span.hpp>
#include <traits.hpp>

namespace io {
namespace detail {
namespace visitors {

struct mdarray_size
    : public boost::static_visitor<std::size_t>
{};

struct mdarray_rank
    : public boost::static_visitor<std::size_t>
{};

struct mdarray_shape
    : public boost::static_visitor<boost::span<const std::size_t>>
{};

struct mdarray_insert
    : public boost::static_visitor<void>
{};

template<typename... SupportedTypes>
struct mdarray_at
    : public boost::static_visitor<
        typename traits::type_list<SupportedTypes...>::variant_scalar
    >
{};



} // namespace visitors
} // namespace detail
} // namespace io

#endif // NGEN_IO_MDFRAME_VISITORS_HPP
