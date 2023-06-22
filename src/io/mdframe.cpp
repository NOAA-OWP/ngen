#include "mdframe.hpp"

namespace io {

// Dimension Member Functions -------------------------------------------------

auto mdframe::find_dimension(const std::string& name) const noexcept
    -> dimension_set::const_iterator
{
    return this->m_dimensions.find(dimension(name));
}

auto mdframe::get_dimension(const std::string& name) const noexcept
    -> boost::optional<detail::dimension>
{
    decltype(auto) pos = this->find_dimension(name);

    boost::optional<detail::dimension> result;
    if (pos != this->m_dimensions.end()) {
        result = *pos;
    }

    return result;
}

bool mdframe::has_dimension(const std::string& name) const noexcept
{
    return this->find_dimension(name) != this->m_dimensions.end();
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

auto mdframe::find_variable(const std::string& name) const noexcept
    -> variable_map::const_iterator
{
    return this->m_variables.find(name);
}

auto mdframe::get_variable(const std::string& name) noexcept
    -> variable&
{
    return this->m_variables[name];
}

auto mdframe::operator[](const std::string& name) noexcept
    -> variable&
{
    return this->get_variable(name);
}

bool mdframe::has_variable(const std::string& name) const noexcept
{
    return this->find_variable(name) != this->m_variables.end();
}


} // namespace io