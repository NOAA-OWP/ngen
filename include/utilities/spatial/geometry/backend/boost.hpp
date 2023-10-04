#pragma once

#include <boost/geometry.hpp>
#include "../backend.hpp"
#include "../../../span.hpp"

namespace bg = boost::geometry;

namespace ngen {
namespace spatial {

struct boost_backend
  : backend_tag
{
    boost_backend() = delete;
};

/**
 * Boost.Geometry Backend
 *
 * @note This backend assumes a planar coordinate representation,
 *       which may lead to inaccuracies when using a geographic
 *       coordinate system, such as EPSG:4326.
 */
template<>
struct backend<boost_backend>
{
    using tag                  = boost_backend;
    using size_type            = backend<void>::size_type;
    using point_type           = bg::model::d2::point_xy<double>;
    using linestring_type      = bg::model::linestring<point_type>;
    using ring_type            = bg::model::ring<point_type>;
    using polygon_type         = bg::model::polygon<point_type>;

    static point_type make_point(double x, double y)
    {
        return point_type{x, y};
    }

    static double get_x(point_type pt)
    {
        return pt.x();
    }

    static double get_y(point_type pt)
    {
        return pt.y();
    }

    static linestring_type make_linestring(boost::span<point_type> points)
    {
        return linestring_type{points.begin(), points.end()};
    }

    static ring_type make_ring(boost::span<point_type> points)
    {
        return ring_type{points.begin(), points.end()};
    }

    static point_type& get_point(linestring_type& line, size_type n)
    {
        return line.at(n);
    }

    static point_type& get_point(ring_type& ring, size_type n)
    {
        return ring.at(n);
    }

    static size_type num_points(linestring_type& line)
    {
        return line.size();
    }

    static size_type num_points(ring_type& ring)
    {
        return ring.size();
    }

    static polygon_type make_polygon(boost::span<linestring_type> rings)
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

    static ring_type& get_ring(polygon_type& p, size_type n)
    {
        return n > 0 ? p.inners().at(n) : p.outer();
    }
};

} // namespace spatial
} // namespace ngen
