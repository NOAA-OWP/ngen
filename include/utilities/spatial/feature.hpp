#ifndef NGEN_UTILITIES_SPATIAL_FEATURE_HPP
#define NGEN_UTILITIES_SPATIAL_FEATURE_HPP

#include <unordered_map>
#include <string>

#include "geometry.hpp"
#include "property.hpp"

namespace ngen {
namespace spatial {

class feature
{
  private:
    std::unique_ptr<geometry> geometry_;
    std::unordered_map<std::string, property> properties_;
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_FEATURE_HPP
