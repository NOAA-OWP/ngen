#ifndef NGEN_UTILITIES_SPATIAL_GEOMETRY_TRAITS_HPP
#define NGEN_UTILITIES_SPATIAL_GEOMETRY_TRAITS_HPP

#include <vector>
#include <type_traits>
#include "span.hpp"

namespace ngen {
namespace spatial {

struct geometry_backend
{};

using point_span = const boost::span<const std::pair<double, double>>;
using line_span = const boost::span<const boost::span<const std::pair<double, double>>>;

template<
    typename Tp = void,
    std::enable_if_t<
        std::is_base_of<geometry_backend, Tp>::value ||
        std::is_void<Tp>::value,
    bool> = true
>
struct backend_traits;

// ----------------------------------------------------------------------------

template<>
struct backend_traits<void>
{
    using tag                  = void;
    using point_type           = std::pair<double, double>;
    using linestring_type      = std::vector<point_type>;
    using polygon_type         = std::vector<linestring_type>;
    using multipoint_type      = std::vector<point_type>;
    using multilinestring_type = std::vector<linestring_type>;
    using multipolygon_type    = std::vector<polygon_type>;

    static point_type make_point(double x, double y)
    {
        return {x, y};
    }

    static linestring_type make_linestring(point_span points)
    {
        return { points.begin(), points.end() };
    }

    static polygon_type make_polygon(line_span rings)
    {
        return { rings.begin(), rings.end() };
    }

    static double getx(const point_type& p)
    {
        return p.first;
    }

    static double gety(const point_type& p)
    {
        return p.second;
    }
};

} // namespace spatial
} // namespace ngen


#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_TRAITS_HPP
