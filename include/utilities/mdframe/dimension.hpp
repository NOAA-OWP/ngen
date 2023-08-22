#ifndef NGEN_IO_MDFRAME_DIMENSION_HPP
#define NGEN_IO_MDFRAME_DIMENSION_HPP

#include <boost/optional.hpp>
#include <utility>

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

    dimension() = default;

    dimension(const dimension& d) = default;
    dimension& operator=(const dimension& d) = default;
    
    dimension(dimension&& d) = default;
    dimension& operator=(dimension&& d) = default;

    ~dimension() = default;

    dimension(const std::string& name)
        : m_name(name)
        , m_size(static_cast<std::size_t>(-1)) {};
    
    dimension(const std::string& name, std::size_t size)
        : m_name(name)
        , m_size(size) {};
    
    bool operator==(const dimension& rhs) const
    {  
        return this->m_name == rhs.m_name;
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
} // namespace ngen

#endif // NGEN_IO_MDFRAME_DIMENSION_HPP
