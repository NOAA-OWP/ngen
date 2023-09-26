#pragma once

#include "traits.hpp"

#include <boost/geometry.hpp>

namespace bg = boost::geometry;

namespace ngen {
namespace spatial {
namespace backend {
struct boost
  : geometry_backend{};
} // namespace backend

template<>
struct backend_traits<backend::boost>
{
    using tag                  = backend::boost;
    using point_type           = bg::model::d2::point_xy<double>;
    using linestring_type      = bg::model::linestring<point_type>;
    using polygon_type         = bg::model::polygon<point_type>;
    using multipoint_type      = bg::model::multi_point<point_type>;
    using multilinestring_type = bg::model::multi_linestring<linestring_type>;
    using multipolygon_type    = bg::model::multi_polygon<polygon_type>;

    static point_type make_point(double x, double y)
    {
        return point_type{x, y};
    }

    static linestring_type make_linestring(ngen::spatial::point_span points)
    {
        return linestring_type{points.begin(), points.end()};
    }

    static polygon_type make_polygon(line_span rings)
    {
        polygon_type p{};
        
        if (rings.size() > 0) {
            const auto& outer = rings.front();
            p.outer().emplace_back(outer.begin(), outer.end());
        }

        if (rings.size() > 1) {
            decltype(auto) inners = rings.subspan(1);
            for (const auto& inner : inners) {
                p.inners().emplace_back(inner.begin(), inner.end());
            }
        }

        return p;
    }
};

} // namespace spatial
} // namespace ngen
