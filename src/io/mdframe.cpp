#include "mdframe.hpp"

namespace io {
namespace detail {

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

// Dimension Member Functions -------------------------------------------------

auto mdframe::get_dimension(const std::string& name) const noexcept
    -> boost::optional<const detail::dimension&>
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

// Variable Member Functions --------------------------------------------------

auto mdframe::get_variable(const std::string& name) const noexcept
    -> boost::optional<variable>
{
   
    decltype(auto) var = this->m_variables.find(variable(name));
    
    boost::optional<variable> result;
    if (var != this->m_variables.end()) {
        result = *var;
    }

    return result;
}

bool mdframe::has_variable(const std::string& name) const noexcept
{
    return this->m_variables.find(name) != this->m_variables.end();
}

mdframe& mdframe::add_variable(const std::string& name)
{
    // should not update if the variable already exists
    this->m_variables.emplace(name, mdvector_variant{});
    return *this;
}

} // namespace io