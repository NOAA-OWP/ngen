#ifndef NGEN_IO_MDFRAME_HPP
#define NGEN_IO_MDFRAME_HPP

#include <algorithm>
#include <unordered_map>
#include <type_traits>
#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include "mdvector.hpp"

namespace io {

namespace traits {

template<typename... Ts>
struct type_list{
    /**
     * Provides a type alias for a boost::variant
     * containing the types of this type list.
     *
     * For example, for a type_list<int, std::string>:
     *
     *   boost::variant<int, std::string>
     */
    using variant_scalar = boost::variant<Ts...>;

    /**
     * Like variant_scalars, but for container types.
     *
     * For example, for a type_list<int, std::string> with @c{Container} -> std::vector:
     *
     *   boost::variant<std::vector<int>, std::vector<std::string>>
     * 
     * @tparam Container Type of container to hold
     *                   each type in within the variant.
     */
    template<template<typename> class Container>
    using variant_container = boost::variant<Container<Ts>...>;

    /**
     * Semantically, we define that a type is support if it's
     * convertible to (not necessarily the same as) any of the
     * supported types.
     *
     * @tparam S type to check
     */
    template<typename S>
    using is_supported = is_convertible_to_any<S, Ts...>;

    /**
     * Used for SFINAE. Enables a template if the given type is
     * supported within this type list.
     *
     * @tparam S type to check
     */
    template<typename S, typename Tp>
    using enable_if_supports = std::enable_if_t<is_supported<S>::value, Tp>;
};

} // namespace traits

/**
 * A multi-dimensional, tagged data frame.
 */
class mdframe {
    using size_type = std::size_t;

    /**
     * The variable value types this frame can support.
     * These are stored as a compile-time type list to
     * derive further type aliases.
     */
    using types = traits::type_list<int, double, bool, std::string>;

    /**
     * A boost::variant type consisting of mdvectors of the
     * support types, i.e. for types int and double, this is
     * equivalent to:
     *     boost::variant<io::mdvector<int>, io::mdvector<double>>
     */
    using vector_variant = types::variant_container<io::mdvector>;

    using dimension_type = std::pair<std::string, boost::optional<size_type>>;

    auto get_dimension(const std::string& name) const noexcept
    {
        const auto pos = std::find(
            this->dimensions.begin(),
            this->dimensions.end(),
            [&name](const dimension_type& p) {
                return name == p.first;
            }
        );

        boost::optional<const dimension_type&> opt;
        if (pos != std::end(this->dimensions)) {
            opt = *pos;
        }
        return opt;
    }

    bool has_dimension(const std::string& name) const noexcept
    {
        return this->get_dimension(name) != boost::none;
    }

    mdframe& add_dimension(const std::string& name)
    {
        if (!this->has_dimension(name)) {
            this->dimensions.push_back({ name, boost::none });
        }
    
        return *this;
    }

  private:
    std::vector<dimension_type> dimensions;
    std::unordered_map<std::string, vector_variant> variables;
};

} // namespace io

#endif // NGEN_IO_MDFRAME_HPP