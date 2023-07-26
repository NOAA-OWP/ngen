#ifndef NGEN_IO_MDFRAME_DIMENSION_HPP
#define NGEN_IO_MDFRAME_DIMENSION_HPP

#include <boost/optional.hpp>

namespace ngen {

class mdframe;

namespace detail {

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

        static std::size_t apply(const dimension& d) noexcept
        {
            return dimension::hash{}(d);
        }
    };

    dimension()
        : m_name()
        , m_size() {};

    dimension(const std::string& name)
        : m_name(name)
        , m_size(static_cast<std::size_t>(-1)) {};
    
    dimension(const std::string& name, std::size_t size)
        : m_name(name)
        , m_size(size) {};

    dimension(const dimension& d)
        : m_name(d.m_name)
        , m_size(d.m_size) {};

    dimension(dimension&& d)
        : m_name(std::move(d.m_name))
        , m_size(std::move(d.m_size)) {};

    dimension& operator=(const dimension& d) {
        this->m_name = d.m_name;
        this->m_size = d.m_size;
        return *this;
    }

    dimension& operator=(dimension&& d) {
        this->m_name = std::move(d.m_name);
        this->m_size = std::move(d.m_size);
        return *this;
    }
    
    bool operator==(const dimension& rhs) const
    {  
        return dimension::hash::apply(*this) == dimension::hash::apply(rhs);
    }

    auto size() const noexcept { return this->m_size; }
    const std::string& name() const noexcept { return this->m_name; }

  private:
    friend mdframe;

    std::string m_name;

    // we declare m_size as mutable so that
    // its size can be updated during construction
    mutable std::size_t m_size;
};

} // namespace detail
} // namespace io

#endif // NGEN_IO_MDFRAME_DIMENSION_HPP
