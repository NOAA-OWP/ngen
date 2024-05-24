#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/core/span.hpp>
#include <boost/geometry.hpp>

#include <geojson/JSONGeometry.hpp>

struct Cell {
    std::uint64_t x = 0;
    std::uint64_t y = 0;
    std::uint64_t z = 0;
    double    value = NAN;

    friend inline bool operator<(const Cell& a, const Cell& b) {
        // TODO: Not really meaningful
        return a.x < b.x || a.y < b.y || a.z < b.z;
    }
};

using box_t = boost::geometry::model::box<geojson::coordinate_t>;

struct BoundingBox
{
    BoundingBox(box_t box)
      : box_(std::move(box))
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

struct SelectorConfig {
    //! Initial time for query.
    //! @todo Refactor to use std::chrono
    time_t init_time;

    //! Duration for query, in seconds.
    //! @todo Refactor to use std::chrono
    long duration_seconds;

    //! Variable to return from query.
    std::string variable_name;

    //! Units for output variable.
    std::string variable_units;
};

struct GridDataSelector {

    //! Cell-based constructor
    GridDataSelector(SelectorConfig config, boost::span<const Cell> cells)
        : config_(std::move(config))
        , cells_(cells.begin(), cells.end()) {
    }

    /**
     * Point-based constructor
     *
     * Constructs a selector taking only the cells
     * from @p grid that correspond to coordinates in @p points.
     *
     * @param config Selector configuration options
     * @param grid Source grid specification
     * @param points Target points used for extraction
     *
     * @todo Implementation
     */
    GridDataSelector(
        SelectorConfig config,
        const GridSpecification& grid,
        boost::span<const geojson::coordinate_t> points
    )
      : config_(std::move(config))
    {
        // Using a std::set since points may be close enough that they are within
        // the same grid cell. This ensures that each cell is uniquely indexed.
        std::set<Cell> cells;
        for (const auto& point : points) {
            cells.emplace(Cell{
                /*x=*/position_(point.get<0>(), grid.extent.xmin(), grid.extent.xmax(), grid.columns),
                /*y=*/position_(point.get<1>(), grid.extent.ymin(), grid.extent.ymax(), grid.rows),
                /*z=*/0UL,
                /*value=*/NAN
            });
        }

        cells_.assign(cells.begin(), cells.end());
    }

    /**
     * Boundary-based constructor
     *
     * Constructs a selector taking only the cells
     * from @p grid that intersect @p polygon.
     *
     * @param config Selector configuration options
     * @param grid Source grid specification
     * @param polygon Target polygon used as mask
     */
    GridDataSelector(
        SelectorConfig config,
        const GridSpecification& grid,
        const geojson::polygon_t& polygon
    )
        : config_(std::move(config))
    {
        const auto xmin = grid.extent.xmin();
        const auto xmax = grid.extent.xmax();
        const auto ymin = grid.extent.ymin();
        const auto ymax = grid.extent.ymax();
        const auto ydiff = static_cast<double>(grid.rows) / (ymax - ymin);
        const auto xdiff = static_cast<double>(grid.columns) / (xmax - xmin);
    
        const auto bbox = BoundingBox{ boost::geometry::return_envelope<box_t>(polygon) };
        for (double row = bbox.ymin(); row < bbox.ymax() - ydiff; row += ydiff) {
            for (double col = bbox.xmin(); col < bbox.xmax() - xdiff; row += xdiff) {
                const box_t cell_box = {
                    /*min_corner=*/{ col, row },
                    /*max_corner=*/{ col + xdiff, row + ydiff }
                };

                if (boost::geometry::intersects(cell_box, polygon)) {
                    auto x = position_(col, xmin, xmax, grid.columns);
                    auto y = position_(row, ymin, ymax, grid.rows);
                    cells_.emplace_back(Cell{x, y, /*z=*/0UL, /*value=*/NAN});
                }
            }
        }
    }

    /**
     * Extent-based constructor
     *
     * @param config Selector configuration options
     * @param grid Source grid specification
     * @param extent Target bounding box used as mask
     */
    GridDataSelector(
      SelectorConfig config,
      const GridSpecification& grid,
      const BoundingBox& extent
    )
      : config_(std::move(config))
    {
        const auto col_min = position_(extent.xmin(), grid.extent.xmin(), grid.extent.xmax(), grid.columns);
        const auto col_max = position_(extent.xmax(), grid.extent.xmin(), grid.extent.xmax(), grid.columns);
        const auto row_min = position_(extent.ymin(), grid.extent.ymin(), grid.extent.ymax(), grid.rows);
        const auto row_max = position_(extent.ymax(), grid.extent.ymin(), grid.extent.ymax(), grid.rows);
        const auto ncells  = (row_max - row_min) * (col_max - col_min);

        cells_.reserve(ncells);
        for (auto row = row_min; row < row_max; row++) {
            for (auto col = col_min; col < col_max; col++) {
                cells_.emplace_back(Cell{/*x=*/col, /*y=*/row, /*z=*/0UL, /*value=*/NAN});
            }
        }
    }

    GridDataSelector() noexcept = default;

    virtual ~GridDataSelector() = default;

    time_t& initial_time() noexcept {
        return config_.init_time;
    }

    time_t initial_time() const noexcept {
        return config_.init_time;
    }

    long& duration() noexcept {
        return config_.duration_seconds;
    }

    long duration() const noexcept {
        return config_.duration_seconds;
    }

    std::string& variable() noexcept {
        return config_.variable_name;
    }

    const std::string& variable() const noexcept {
        return config_.variable_name;
    }

    std::string& units() noexcept {
        return config_.variable_units;
    }

    const std::string& units() const noexcept {
        return config_.variable_units;
    }

    boost::span<Cell> cells() noexcept {
        return cells_;
    }

    boost::span<const Cell> cells() const noexcept {
        return cells_;
    }

  private:
    static std::uint64_t position_(double position, double min, double max, std::uint64_t upper_bound) {
        if (position < min || position > max) {
            return static_cast<std::uint64_t>(-1);
        }

        return std::floor((position - min) * (static_cast<double>(upper_bound) / (max - min)));
    }

    //! General selector configuration
    SelectorConfig config_;

    //! Cells to gather.
    std::vector<Cell> cells_;
};
