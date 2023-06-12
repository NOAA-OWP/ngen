#include "mdframe.hpp"

namespace io {

namespace detail {

std::size_t dimension::hash::operator()(const dimension& d) const noexcept {
        return std::hash<std::string>{}(d.m_name);
    }

dimension::dimension(const std::string& name)
    : m_name(name)
    , m_size(boost::none) {};

dimension::dimension(const std::string& name, std::size_t size)
    : m_name(name)
    , m_size(size) {};

bool dimension::operator==(const dimension& rhs) const
{
    return this->m_name == rhs.m_name;
}

} // namespace detail


// Dimension Member Functions

boost::optional<const detail::dimension&> mdframe::get_dimension(const std::string& name) const noexcept
{
    decltype(auto) pos = this->m_dimensions.find(name);

    boost::optional<const detail::dimension&> result;
    if (pos != this->m_dimensions.end()) {
        result = *pos;
    }

    return result;
}

bool mdframe::has_dimension(const std::string& name) const noexcept
{
    return this->m_dimensions.find(name) != this->m_dimensions.end();
}

mdframe& mdframe::add_dimension(const std::string& name)
{
    this->m_dimensions.emplace(name);
    return *this;
}

mdframe& mdframe::add_dimension(const std::string& name, std::size_t size)
{
    auto stat = this->m_dimensions.emplace(name, size);

    if (!stat.second) {
        stat.first->m_size = size;
    }

    return *this;
}

// Variable Member Functions



} // namespace io