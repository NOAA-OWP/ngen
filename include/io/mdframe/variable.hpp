#ifndef NGEN_IO_MDFRAME_VARIABLE_HPP
#define NGEN_IO_MDFRAME_VARIABLE_HPP

#include "mdvector.hpp"

#define MDVECTOR_VISITOR(name, return_type) struct name : boost::static_visitor<return_type>
#define MDVECTOR_VISITOR_TEMPLATE_IMPL(var_name) \
    template<typename T> \
    auto operator()(mdvector<T> var_name) const noexcept

#define MDVECTOR_VISITOR_IMPL(prototype) \
    template<typename T> \
    auto operator()(prototype) const noexcept \

namespace io {

struct dimension;

namespace detail {

namespace visitors {

MDVECTOR_VISITOR(mdvector_true_size, std::size_t)
{
    MDVECTOR_VISITOR_TEMPLATE_IMPL(v) -> std::size_t { return v.true_size(); }
};

MDVECTOR_VISITOR(mdvector_rank, std::size_t)
{
    MDVECTOR_VISITOR_TEMPLATE_IMPL(v) -> std::size_t { return v.rank(); }
};

MDVECTOR_VISITOR(mdvector_capacity, std::size_t)
{
    MDVECTOR_VISITOR_TEMPLATE_IMPL(v) -> std::size_t { return v.capacity(); }
};

} // namespace visitors

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

        static std::size_t apply(const variable& d) noexcept
        {
            return variable::hash{}(d);
        }
    };

    variable() noexcept
        : m_name()
        , m_data()
        , m_dimensions() {};

    variable(const std::string& name) noexcept
        : m_name(name)
        , m_data()
        , m_dimensions(){};

    variable(const std::string& name, const std::vector<dimension>& dimensions)
        : m_name(name)
        , m_data()
        , m_dimensions(dimensions) {};

    variable(const std::string& name, std::initializer_list<dimension> dimensions)
        : m_name(name)
        , m_data()
        , m_dimensions(dimensions) {};

    const mdvector_variant& values() const noexcept
    {
        return this->m_data;
    }

    bool operator==(const variable& rhs) const
    {
        return hash{}(*this) == hash{}(rhs);
    }

    size_type size() const noexcept {
        return boost::apply_visitor(visitors::mdvector_true_size{}, this->m_data);
    }

    size_type rank() const noexcept {
        return boost::apply_visitor(visitors::mdvector_rank{}, this->m_data);
    }

    size_type capacity() const noexcept {
        return boost::apply_visitor(visitors::mdvector_capacity{}, this->m_data);
    }

  private:
    // Name of this variable
    mutable std::string m_name;

    // multi-dimensional vector associated with this variable
    mdvector_variant m_data;

    // References to dimensions that this variable spans
    std::vector<dimension> m_dimensions;
};

} // namespace detail
} // namespace io

#endif // NGEN_IO_MDFRAME_VARIABLE_HPP