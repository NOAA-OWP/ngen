#ifndef NGEN_SPATIAL_FEATURE_HPP
#define NGEN_SPATIAL_FEATURE_HPP

#include <memory>
#include "geometry/geometry.hpp"
#include "property.hpp"

namespace ngen {
namespace spatial {

//! Spatial Feature
//!
//! @note This class owns its associated geometry class.
struct feature : public std::enable_shared_from_this<feature>
{
    using geometry_type = ngen::spatial::geometry;
    using mapped_type   = property;

    mapped_type&       operator[](const std::string& key) noexcept;
    const mapped_type& operator[](const std::string& key) const noexcept;

    geometry_type&       geometry() noexcept;
    const geometry_type& geometry() const noexcept;

    //! Check if a feature contains a specified property
    //! @param key Key of the property
    //! @return true if property exists, false otherwise.
    bool has(const std::string& key) const noexcept;

  private:
    std::unique_ptr<geometry_type> geometry_;
    property_map                   properties_;
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_FEATURE_HPP
