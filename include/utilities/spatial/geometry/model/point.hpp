#ifndef NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_POINT_HPP
#define NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_POINT_HPP

#include "../types.hpp"
#include "../backend.hpp"
#include <utility>

namespace ngen {
namespace spatial {

template<typename BackendPolicy>
struct point
{
    using backend         = ngen::spatial::backend<BackendPolicy>;
    using value_type      = typename backend::value_type;
    using reference       = typename backend::coord_reference;
    using const_reference = typename backend::const_coord_reference;

    point(value_type x, value_type y)
      : data_(backend::make_point(x, y)){};
    
    //! Get reference to X coordinate
    reference x() noexcept
    {
        return backend::get_x(data_);
    };

    //! Get (const) reference to X coordinate
    const_reference x() const noexcept
    {
        return backend::get_x(data_);
    }

    //! Get reference to Y coordinate
    reference y() noexcept
    {
        return backend::get_y(data_);
    }

    //! Get (const) reference to Y coordinate
    const_reference y() const noexcept
    {
        return backend::get_y(data_);
    }

  private:
    typename backend::point_type data_;
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_MODEL_POINT_HPP
