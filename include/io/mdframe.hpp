#ifndef NGEN_IO_MDFRAME_HPP
#define NGEN_IO_MDFRAME_HPP

#include <algorithm>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include "mdvector.hpp"

namespace io {

class mdframe;

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

namespace detail
{

/**
 * Dimension Key
 * 
 * Provides a tagged dimension structure,
 * with an optional size constraint.
 */
struct dimension {

    struct hash
    {
        std::size_t operator()(const dimension& d) const noexcept
        {
            return std::hash<std::string>{}(d.m_name);
        }
    };

    dimension(const std::string& name);
    dimension(const std::string& name, std::size_t size);
    bool operator==(const dimension& rhs) const;

  private:
    friend mdframe;

    std::string m_name;

    // we declare m_size as mutable so that
    // its size can be updated during construction
    mutable boost::optional<std::size_t> m_size;
};

/**
 * Variable Key
 *
 * Provides a tagged variable structure.
 * 
 * @tparam SupportedTypes types that this variable is able to hold.
 */
template<typename... SupportedTypes>
struct variable {
    using size_type = std::size_t;

    /**
     * The variable value types this frame can support.
     * These are stored as a compile-time type list to
     * derive further type aliases.
     */
    using types = traits::type_list<SupportedTypes...>;

    /**
     * A boost::variant type consisting of mdvectors of the
     * support types, i.e. for types int and double, this is
     * equivalent to:
     *     boost::variant<io::mdvector<int>, io::mdvector<double>>
     */
    using mdvector_variant = typename types::template variant_container<io::mdvector>;

    struct hash
    {
        std::size_t operator()(const variable& v) const noexcept
        {
            return std::hash<std::string>{}(v.m_name);
        }
    };

    variable(const std::string& name) noexcept;

    const mdvector_variant& values() const noexcept;

    bool operator==(const variable& rhs) const;

  private:
    // Name of this variable
    mutable std::string m_name;

    // multi-dimensional vector associated with this variable
    mdvector_variant m_data;

    // References to dimensions that this variable spans
    std::vector<std::reference_wrapper<dimension>> m_dimensions;
};

}

/**
 * A multi-dimensional, tagged data frame.
 *
 * This structure (somewhat) mimics the conceptual format
 * of NetCDF. In particular, we can think of an mdframe
 * as a multimap between dimension tuples to multi-dimensional arrays.
 * In other words, it is similar to an R data.frame or pandas dataframe
 * where the column types are multi-dimensional arrays.
 *
 * Heterogenous data types are handled via boost::variant types
 * over mdvector types. As a result, each column
 * is represented by a contiguous block of memory
 * (since mdvector is backed by a std::vector).
 *
 * @section representation Frame Representation
 * Consider the dimensions: x, y, z; and
 * the variables:
 *   - v1<std::string>(x)
 *   - v2<double>(x, y)
 *   - v3<int>(x, y, z)
 *
 * Then, it follows that the corresponding mdframe (spanned over x):
 *
 *   x |  v1 |      v2 |      v3
 * --- | --- | ------- | -------
 *   0 | s_0 | [...]_0 | [[...]]_0
 *   1 | s_1 | [...]_1 | [[...]]_1
 * ... | ... |  ...    |   ...
 *   n | s_n | [...]_n | [[...]]_n
 *
 * where:
 * - v1: Vector of strings,  rank 1 -> [...]
 * - v2: Matrix of doubles,  rank 2 -> [[...]]
 * - v3: Tensor of integers, rank 3 -> [[[...]]]
 *
 * Alternatively, we can project down to a 2D representation by
 * unpacking the dimensions, such that:
 *
 *      dimensions ||                variables
 * =============== || ========================
 *   x |   y |   z ||   v1 |     v2 |       v3
 * --- | --- | --- || ---- | ------ | --------
 *   n |   m |   p || s[n] | d[n,m] | i[n,m,p]
 *
 * > **note:** this is the physical representation
 * >           of how the mdvectors store the data.
 */
class mdframe {
    
    using dimension = detail::dimension;
    using dimension_set = std::unordered_set<dimension, dimension::hash>;

    using variable  = detail::variable<int, double, bool, std::string>;
    using variable_set = std::unordered_set<variable, variable::hash>;

    using size_type = variable::size_type;

    /**
     * The variable value types this frame can support.
     * These are stored as a compile-time type list to
     * derive further type aliases.
     * @see detail::variable
     */
    using types = variable::types;

    using mdvector_variant = variable::mdvector_variant;

    // ------------------------------------------------------------------------
    // Dimension Member Functions
    // ------------------------------------------------------------------------

    /**
     * Return reference to a dimension if it exists. Otherwise, returns boost::none.
     * 
     * @param name Name of the dimension
     * @return boost::optional<const detail::dimension&> 
     */
    auto get_dimension(const std::string& name) const noexcept
        -> boost::optional<const detail::dimension&>;

    /**
     * Check if a dimension exists.
     * 
     * @param name Name of the dimension
     * @return true if the dimension exists in this mdframe.
     * @return false if the dimension does **not** exist in this mdframe.
     */
    bool has_dimension(const std::string& name) const noexcept;

    /**
     * Add a dimension with *unlimited* size to this mdframe.
     * 
     * @param name Name of the dimension.
     * @return mdframe& 
     */
    mdframe& add_dimension(const std::string& name);

    /**
     * Add a dimension with a specified size to this mdframe,
     * or update an existing dimension.
     * 
     * @param name Name of the dimension.
     * @param size Size of the dimension.
     * @return mdframe& 
     */
    mdframe& add_dimension(const std::string& name, std::size_t size);

    // ------------------------------------------------------------------------
    // Variable Member Functions
    // ------------------------------------------------------------------------

    /**
     * Return reference to a mdvector representing a variable, if it exists.
     * Otherwise, returns boost::none.
     * 
     * @param name Name of the variable.
     * @return boost::optional<boost::variant<mdvector>>
     *         (aka, potentially, a variant of supported mdvector types)
     */
    auto get_variable(const std::string& name) const noexcept
        -> boost::optional<variable>;

    /**
     * Check if a variable exists.
     * 
     * @param name Name of the variable.
     * @return true if the variable exists in this mdframe.
     * @return false if the variables does **not** exist in this mdframe.
     */
    bool has_variable(const std::string& name) const noexcept;

    /**
     * Add a (scalar) variable definition to this mdframe.
     * 
     * @param name Name of the variable.
     * @return mdframe& 
     */
    mdframe& add_variable(const std::string& name);

    
    /**
     * Add a (vector) variable definition to this mdframe.
     *
     * @note This member function is constrained by @c{Args}
     *       being constructible to a std::initializer_list of std::string
     *       (i.e. a list of std::string).
     * 
     * @tparam Args list of std::string representing dimension names.
     * @param name Name of the variable.
     * @param dimensions Names of the dimensions this variable spans.
     * @return mdframe&
     */
    template<typename... Args, std::enable_if_t<
        std::is_constructible<std::initializer_list<std::string>, Args...>::value,
        bool> = true>
    mdframe& add_variable(const std::string& name, Args&&... dimensions)
    {

    }

    /**
     * Add a (vector) variable definition to this mdframe, with an initial value.
     *
     * @note This member function is constrained by @c{T} being
     *       a supported type, and @c{Args} being constructible
     *       to a std::initializer_list of std::string (i.e. a list of std::string).
     *
     * @tparam T supported types of this mdframe.
     * @tparam Args list of std::string representing dimension names.
     * @param name Name of the variable.
     * @param values Values to push into this variable.
     * @param dimennsions Names of the dimensions this variable spans.
     * @return mdframe& 
     */
    template<
        typename T,
        typename... Args,
        types::enable_if_supports<T, bool> = true,
        std::enable_if_t<
            std::is_constructible<
                std::initializer_list<std::string>,
                Args...
            >::value, bool
        > = true
    >
    mdframe& add_variable(const std::string& name, const mdvector<T>& values, Args&&... dimensions);

  private:
    dimension_set m_dimensions;
    variable_set  m_variables;
};

} // namespace io

#endif // NGEN_IO_MDFRAME_HPP