#ifndef NGEN_SPATIAL_FEATURE_HPP
#define NGEN_SPATIAL_FEATURE_HPP

#include <memory>
#include "geometry/geometry.hpp"
// #include "property.hpp"

namespace ngen {
namespace spatial {

//! Spatial Feature
//!
//! @note This class owns its associated geometry class.
struct feature
{
  private:
    std::unique_ptr<geometry> geometry_;
    // property_map              properties_;
};

struct feature_collection;

} // namespace spatial
} // namespace ngen

#endif // NGEN_SPATIAL_FEATURE_HPP
