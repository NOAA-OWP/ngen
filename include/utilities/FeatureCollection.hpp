#pragma once

#include <vector>

#include "Feature.hpp"

namespace ngen {

struct FeatureCollection
  : private std::vector<Feature> 
{
    using base_type = std::vector<Feature>;
    using base_type::vector;
    using base_type::at;
    using base_type::operator[];
    using base_type::push_back;
    using base_type::emplace_back;
    using base_type::size;
    using base_type::begin;
    using base_type::end;
    using base_type::front;
    using base_type::back;
    using base_type::reserve;

    void swap(FeatureCollection& other) noexcept
    {
        base_type::swap(other);
    }
};

} // namespace ngen
