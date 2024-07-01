#pragma once

#include <cstdint>
#include <string>
#include <boost/geometry.hpp>
#include <geojson/JSONGeometry.hpp>

using box_t = boost::geometry::model::box<geojson::coordinate_t>;

struct BoundingBox
{
    BoundingBox() = default;

    BoundingBox(double xmin, double xmax, double ymin, double ymax)
      : box_(/*min_corner=*/{xmin, ymin}, /*max_corner=*/{xmax, ymax})
    {}

    explicit BoundingBox(box_t box)
      : box_(box)
    {}

    template<typename Geometry>
    explicit BoundingBox(const Geometry& geom)
      : box_(boost::geometry::return_envelope<box_t>(geom))
    {}

    double xmin() const noexcept
    {
        return box_.min_corner().get<0>();
    }

    double xmax() const noexcept
    {
        return box_.max_corner().get<0>();
    }

    double ymin() const noexcept
    {
        return box_.min_corner().get<1>();
    }

    double ymax() const noexcept
    {
        return box_.max_corner().get<1>();
    }

    geojson::polygon_t as_polygon() const noexcept {
        boost::geometry::box_view<box_t> view{box_};
        geojson::polygon_t poly;
        poly.outer() = { view.begin(), view.end() };
        return poly;
    }

    const box_t& as_box() const noexcept {
        return box_;
    }

  private:
    box_t box_;
};

struct GridSpecification {
    //! Total number of rows (aka y)
    std::uint64_t rows;

    //! Total number of columns (aka x)
    std::uint64_t columns;

    //! Extent of the grid region (aka min-max corner points)
    BoundingBox extent;
};

struct GridDataSelector {
    std::string variable_name;
    time_t init_time;
    long duration;
    std::string output_units;

  private:
    static std::uint64_t position_(double position, double min, double max, std::uint64_t upper_bound) {
        if (position < min || position > max) {
            return static_cast<std::uint64_t>(-1);
        }

        return std::floor((position - min) * (static_cast<double>(upper_bound) / (max - min)));
    }
};
