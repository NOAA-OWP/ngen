#ifndef NGEN_UTILITIES_TRAITS_HPP
#define NGEN_UTILITIES_TRAITS_HPP

#include <type_traits>
#include <boost/variant.hpp>

namespace ngen {
namespace traits {

template<bool...>
struct bool_pack
{};

template<bool B>
using bool_constant = std::integral_constant<bool, B>;

// a C++17 conjunction impl
template<bool... Bs>
using conjunction = std::is_same<bool_pack<true, Bs...>, bool_pack<Bs..., true>>;

// a C++17 disjunction impl
template<bool... Bs>
using disjunction = bool_constant<!conjunction<!Bs...>::value>;

/**
    * Check that all types @c{Ts} are the same as @c{T}.
    * 
    * @tparam T Type to constrain to
    * @tparam Ts Types to check
    */
template<typename T, typename... Ts>
using all_is_same = conjunction<std::is_same<Ts, T>::value...>;

/**
    * Checks that all types @c{From} are convertible to @c{T}
    * 
    * @tparam To Type to constrain to
    * @tparam From Types to check
    */
template<typename To, typename... From>
using all_is_convertible = conjunction<std::is_convertible<From, To>::value...>;

/**
    * Checks that @c{From} is convertible to any types in @c{To}
    * 
    * @tparam From Type to constrain to
    * @tparam To Types to check
    */
template<typename From, typename... To>
using is_convertible_to_any = disjunction<std::is_convertible<From, To>::value...>;

/**
    * Checks that @c{T} is the same as at least one of @c{Ts}
    * 
    * @tparam T Type to check
    * @tparam Ts Types to contrain to
    */
template<typename T, typename... Ts>
using is_same_to_any = disjunction<std::is_same<T, Ts>::value...>;

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
     * Semantically, we define that a type is supported if it's
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
} // namespace io

#endif // NGEN_IO_TRAITS_HPP
