#ifndef NGEN_UTILITIES_SPATIAL_FEATURE_HPP
#define NGEN_UTILITIES_SPATIAL_FEATURE_HPP

#include <unordered_map>
#include <string>

#include "property.hpp"

namespace ngen {
namespace spatial {

struct geometry;

/**
 * Spatial Feature
 *
 * @details Provides an interface to a spatial "feature".
 *          A feature is a geospatial model around geometry,
 *          spatial referencing, and additional properties.
 * 
 */
class feature
{
  using size_type          = std::size_t;
  using geometry_type      = ngen::spatial::geometry;
  using geometry_ptr       = std::unique_ptr<geometry_type>;
  using const_geometry_ptr = const std::unique_ptr<const geometry_type>;
  using property_type      = ngen::spatial::property;

  property&           operator[](const std::string& key)       noexcept;
  const property&     operator[](const std::string& key) const noexcept;
  bool                contains(const std::string& key)   const noexcept;
  geometry_ptr&       geometry()                               noexcept;
  const_geometry_ptr& geometry()                         const noexcept;

  private:
    std::string  id_;
    size_type    srid_;
    geometry_ptr geometry_;
    std::unordered_map<std::string, property> properties_;
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_FEATURE_HPP
