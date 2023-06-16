#ifndef NGEN_IO_MDFRAME_VARIABLE_HPP
#define NGEN_IO_MDFRAME_VARIABLE_HPP

#include "mdarray.hpp"
#include "traits.hpp"

#define MDARRAY_VISITOR(name, return_type) struct name : boost::static_visitor<return_type>
#define MDARRAY_VISITOR_TEMPLATE_IMPL(var_name) \
    template<typename T> \
    auto operator()(mdarray<T> var_name) const noexcept

#define MDARRAY_VISITOR_IMPL(prototype) \
    template<typename T> \
    auto operator()(prototype) const noexcept \

namespace io {

struct dimension;

namespace detail {

namespace visitors { // -------------------------------------------------------

#warning NO DOCUMENTATION
MDARRAY_VISITOR(mdarray_size, std::size_t)
{
    MDARRAY_VISITOR_TEMPLATE_IMPL(v) -> std::size_t { return v.size(); }
};

#warning NO DOCUMENTATION
MDARRAY_VISITOR(mdarray_rank, std::size_t)
{
    MDARRAY_VISITOR_TEMPLATE_IMPL(v) -> std::size_t { return v.rank(); }
};

} // namespace visitors -------------------------------------------------------

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
    using mdarray_variant = typename types::template variant_container<io::mdarray>;

    #warning NO DOCUMENTATION
    struct hash
    {
        std::size_t operator()(const variable& v) const noexcept
        {
            return std::hash<std::string>{}(v.m_name);
        }

        static std::size_t apply(const variable& d) noexcept
        {
            return variable::hash{}(d);
        }
    };

    /**
     * Constructs an empty variable
     */
    variable() noexcept
        : m_name()
        , m_data()
        , m_dimensions() {};

    /**
     * Constructs an empty named variable
     *
     * @param name Name of the dimension
     */
    variable(const std::string& name) noexcept
        : m_name(name)
        , m_data()
        , m_dimensions(){};

    /**
     * Constructs a named variable spanned over the given dimensions
     *
     * @param name Name of the dimension
     * @param dimensions List of dimensions
     */
    variable(const std::string& name, const std::vector<dimension>& dimensions)
        : m_name(name)
        , m_data()
        , m_dimensions(dimensions) {};

    /**
     * Constructs a named variable spanned over the given dimensions
     *
     * @param name Name of the dimension
     * @param dimensions List of dimensions
     */
    variable(const std::string& name, std::initializer_list<dimension> dimensions)
        : m_name(name)
        , m_data()
        , m_dimensions(dimensions) {};

    /**
     * Get the values of this variable
     * 
     * @return const mdvector_variant& 
     */
    const mdarray_variant& values() const noexcept
    {
        return this->m_data;
    }

    #warning NO DOCUMENTATION
    bool operator==(const variable& rhs) const
    {
        return hash{}(*this) == hash{}(rhs);
    }

    /**
     * Get the (true) size of this variable
     *
     * @see mdvector::true_size
     * 
     * @return size_type 
     */
    size_type size() const noexcept {
        return boost::apply_visitor(visitors::mdarray_size{}, this->m_data);
    }

    /**
     * Get the rank of this variable
     *
     * @see mdvector::rank
     * 
     * @return size_type 
     */
    size_type rank() const noexcept {
        return boost::apply_visitor(visitors::mdarray_rank{}, this->m_data);
    }

  private:
    // Name of this variable
    mutable std::string m_name;

    // multi-dimensional vector associated with this variable
    mdarray_variant m_data;

    // References to dimensions that this variable spans
    std::vector<dimension> m_dimensions;
};

} // namespace detail
} // namespace io

#endif // NGEN_IO_MDFRAME_VARIABLE_HPP