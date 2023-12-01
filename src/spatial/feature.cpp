#include <feature.hpp>

namespace ngen {
namespace spatial {

auto feature::operator[](const std::string& key) noexcept
  -> mapped_type&
{
    return properties_[key];
}

auto feature::operator[](const std::string& key) const noexcept
  -> const mapped_type&
{
    return properties_[key];
}

auto feature::geometry() noexcept
  -> geometry_type&
{
    return *geometry_.get();
}

auto feature::geometry() const noexcept
  -> const geometry_type&
{
    return *geometry_.get();
}

} // namespace spatial
} // namespace ngen
