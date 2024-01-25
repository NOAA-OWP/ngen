#pragma once

#include <boost/variant.hpp>
#include <boost/geometry.hpp>
#include <boost/json.hpp>

namespace ngen {

using point           = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
using linestring      = boost::geometry::model::linestring<point>;
using polygon         = boost::geometry::model::polygon<point>;
using multipoint      = boost::geometry::model::multi_point<point>;
using multilinestring = boost::geometry::model::multi_linestring<linestring>;
using multipolygon    = boost::geometry::model::multi_polygon<polygon>;
using geometry        = boost::variant<point, linestring, polygon, multipoint, multilinestring, multipolygon>;

struct Feature {
    using map_type        = boost::json::object;
    using geometry_type   = ngen::geometry;
    using key_type        = map_type::key_type;
    using mapped_type     = map_type::mapped_type;
    using reference       = mapped_type&;
    using const_reference = const mapped_type&;
    using iterator        = map_type::iterator;
    using const_iterator  = map_type::const_iterator;

    reference operator[](key_type key) noexcept;
    const_reference operator[](key_type key) const noexcept;
    reference at(key_type key);
    const_reference at(key_type key) const noexcept;
    geometry_type& geometry() noexcept;
    const geometry_type& geometry() const noexcept;
    iterator begin() noexcept;
    iterator end() noexcept;
    void swap(Feature& feature) noexcept;

  private:
    geometry_type geometry_;
    map_type      properties_;
};

} // namespace ngen
