#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/core/span.hpp>

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

struct Extent {
    double xmin;
    double xmax;
    double ymin;
    double ymax;

    geojson::polygon_t as_polygon() const noexcept {
        geojson::polygon_t result;
        result.outer().reserve(4);
        result.outer().emplace_back(xmin, ymin);
        result.outer().emplace_back(xmax, ymin);
        result.outer().emplace_back(xmax, ymax);
        result.outer().emplace_back(xmin, ymax);
        return result;
    }
};

struct GridSpecification {
    //! Total number of rows (aka y)
    std::uint64_t rows;

    //! Total number of columns (aka x)
    std::uint64_t columns;

    //! Extent of the grid region (aka min-max corner points)
    Extent extent;
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
    GridDataSelector(SelectorConfig config, boost::span<const Cell> cells) noexcept
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
    ) noexcept
      : config_(std::move(config))
    {
        // Using a std::set since points may be close enough that they are within
        // the same grid cell. This ensures that each cell is uniquely indexed.
        std::set<Cell> cells;
        for (const auto& point : points) {
            cells.emplace(Cell{
                /*x=*/position_(point.get<0>(), grid.extent.xmin, grid.extent.xmax, grid.columns),
                /*y=*/position_(point.get<1>(), grid.extent.ymin, grid.extent.ymax, grid.rows),
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
     *
     * @todo Sweep line or ray casting to get polygon grid cells
     */
    // GridDataSelector(
    //     SelectorConfig config,
    //     const GridSpecification& grid,
    //     const geojson::polygon_t& polygon
    // )
    //     : config_(std::move(config))
    // {
    //     static constexpr auto epsilon = 1e-7;
    // 
    //     std::set<Cell> boundary;
    //     const auto ydiff = static_cast<double>(grid.rows) / (grid.extent.ymax - grid.extent.ymin);
    //     const auto xdiff = static_cast<double>(grid.columns) / (grid.extent.xmax - grid.extent.xmin);
    //     const auto bbox = bounding_box_(polygon);
    // 
    //     for (const auto& p : polygon.outer()) {
    //         const auto x_index = position_(p.get<0>(), grid.extent.xmin, grid.extent.xmax, grid.columns);
    //         const auto y_index = position_(p.get<1>(), grid.extent.ymin, grid.extent.ymax, grid.rows);
    //         const auto set_pair = boundary.emplace(x_index, y_index, 0, NAN);
    //     }
    // 
    //     throw std::runtime_error{"Boundary-constructor not implemented"};
    // }

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
      const Extent& extent
    ) noexcept
      : config_(std::move(config))
    {
        const auto col_min = position_(extent.xmin, grid.extent.xmin, grid.extent.xmax, grid.columns);
        const auto col_max = position_(extent.xmax, grid.extent.xmin, grid.extent.xmax, grid.columns);
        const auto row_min = position_(extent.ymin, grid.extent.ymin, grid.extent.ymax, grid.rows);
        const auto row_max = position_(extent.ymax, grid.extent.ymin, grid.extent.ymax, grid.rows);
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
    //! Returns true if point is inside polygon or on its boundary.
    //! @note may not be needed
    static bool intersects_(const geojson::coordinate_t& point, const geojson::polygon_t& polygon) {
        const auto& boundary = polygon.outer();
        const auto px        = point.get<0>();
        const auto py        = point.get<1>();
        size_t intersects    = 0;

        for (size_t i = 0; i < boundary.size() - 1; i++) {
            const auto& current_bpoint = boundary[i];
            const auto& next_bpoint    = boundary[i + 1];
            const double ix            = current_bpoint.get<0>();
            const double iy            = current_bpoint.get<1>();
            const double jx            = next_bpoint.get<0>();
            const double jy            = next_bpoint.get<1>();
            const double idx           = px - ix;
            const double idy           = py - iy;
            const double jdx           = px - jx;
            const double jdy           = py - jy;
            const double crossing      = ((idx - jdx) * idy) - (idx * (idy - jdy));

            if (crossing == 0 && idx * jdx <= 0 and idy * jdy <= 0) {
                return true;
            }

            if ((idy >= 0 && jdy < 0) || (jdy >= 0 && idy < 0)) {
                if (crossing > 0) {
                    intersects++;
                } else if (crossing < 0) {
                    intersects--;
                }
            }
        }

        return intersects != 0;
    }

    static Extent bounding_box_(const geojson::polygon_t& polygon) {
        Extent bbox{
            /*xmin=*/std::numeric_limits<double>::max(),
            /*xmax=*/std::numeric_limits<double>::lowest(),
            /*ymin=*/std::numeric_limits<double>::max(),
            /*ymax=*/std::numeric_limits<double>::lowest()
        };

        for (const auto& point : polygon.outer()) {
            const auto xcoord = point.get<0>();

            if (xcoord < bbox.xmin) {
                bbox.xmin = xcoord;
            }

            if (xcoord > bbox.xmax) {
                bbox.xmax = xcoord;
            }

            const auto ycoord = point.get<1>();

            if (ycoord < bbox.ymin) {
                bbox.ymin = ycoord;
            }

            if (ycoord > bbox.ymax) {
                bbox.ymax = ycoord;
            }
        }

        return bbox;
    }

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
