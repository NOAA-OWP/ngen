#ifndef NGEN_UTILITIES_SPATIAL_GEOMETRY_BACKEND_HPP
#define NGEN_UTILITIES_SPATIAL_GEOMETRY_BACKEND_HPP

#include <type_traits>

namespace ngen {
namespace spatial {

/**
 * Geometry Backend Tag
 *
 * Non-instantiatable base class for struct tagging.
 * All backend tags should inherit from this tag for
 * use with the backend_traits class.
 */
struct backend_tag{backend_tag() = delete;};

/**
 * Geometry Backend Traits
 * 
 * Specializations must implement the following:
 * 
 * using tag;
 * using value_type;
 * using size_type;
 * using coord_reference;
 * using const_coord_reference;
 * using point_type;
 * using linestring_type;
 * using ring_type;
 * using polygon_type;
 *
 * Points ---------------------------------------------------------------------
 * point_type      make_point(value_type x, value_type y);
 * coord_reference get_x(point_type) noexcept;
 * coord_reference get_y(point_type) noexcept;
 * ----------------------------------------------------------------------------
 *
 * LineStrings ----------------------------------------------------------------
 * linestring_type make_linestring(boost::span<point_type>);
 * ring_type       make_ring(boost::span<point_type>);
 * point_type&     get_point(linestring_type&, size_type);
 * size_type       num_points(linestring_type&) noexcept;
 * point_type&     get_point(ring_type&, size_type);
 * size_type       num_points(ring_type&) noexcept;
 * ----------------------------------------------------------------------------
 *
 * Polygons -------------------------------------------------------------------
 * polygon_type make_polygon(boost::span<linestring_type>);
 * ring_type&   get_ring(polygon_type&, size_type);
 * size_type    num_rings(polygon_type&) noexcept;
 * ----------------------------------------------------------------------------
 */
template<
    typename Tp = void,
    std::enable_if_t<
        std::is_base_of<backend_tag, Tp>::value ||
        std::is_void<Tp>::value,
    bool> = true
>
struct backend
{
    using tag       = void;
    using size_type = std::size_t;
};

} // namespace spatial
} // namespace ngen


#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_BACKEND_HPP
