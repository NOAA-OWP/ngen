#pragma once

#include <NGenConfig.h>

#if NGEN_WITH_PYTHON

#include <forcing/ForcingsEngineDataProvider.hpp>
#include <forcing/GriddedDataSelector.hpp>
#include <geojson/JSONGeometry.hpp>

namespace data_access {

struct GridSpecification
{
    //! Total number of rows (aka y)
    std::uint64_t rows;

    //! Total number of columns (aka x)
    std::uint64_t columns;

    //! Extent of the grid region (aka min-max corner points)
    geojson::box_t extent;
};

struct GridMask {
    //! Geographic extent of the mask region (aka min-max corner points)
    geojson::box_t extent;

    //! Gridded extent of the mask region (aka min-max corner indices)
    std::uint64_t rmin;
    std::uint64_t cmin;
    std::uint64_t rmax;
    std::uint64_t cmax;

    constexpr std::uint64_t rows() const noexcept
    {
        return rmax - rmin + 1;
    }

    constexpr std::uint64_t columns() const noexcept
    {
        return cmax - cmin + 1;
    }

    constexpr std::uint64_t size() const noexcept
    {
        return rows() * columns();
    }

    static constexpr auto bad_position = static_cast<std::uint64_t>(-1);

    //! Derive a discrete position along a real axis from a coordinate component
    //! @param component Coordinate component value
    //! @param lower_bound Lower bound of component's axis
    //! @param upper_bound Upper bound of component's axis
    //! @param cardinality Total number of elements along the discrete axis
    //! @returns A position along the [0, cardinality) discrete axis.
    static std::uint64_t position(double component, double lower_bound, double upper_bound, std::uint64_t cardinality)
    {
        if (component < lower_bound || component > upper_bound) {
            return bad_position;
        }

        // We can use a static_cast instead of std::floor because the position will never be negative,
        // so truncating and flooring are equivalent here.
        return static_cast<std::uint64_t>(
            (component - lower_bound) * (cardinality / (upper_bound - lower_bound))
        );
    }
};


struct ForcingsEngineGriddedDataProvider final :
  public ForcingsEngineDataProvider<double, GriddedDataSelector>
{
    using base_type = ForcingsEngineDataProvider<data_type, selection_type>;

    ~ForcingsEngineGriddedDataProvider() override = default;

    ForcingsEngineGriddedDataProvider(
        const std::string& init,
        std::size_t time_begin_seconds,
        std::size_t time_end_seconds,
        geojson::box_t mask
    );

    data_type get_value(
        const selection_type& selector,
        data_access::ReSampleMethod m
    ) override;

    std::vector<data_type> get_values(
        const selection_type& selector,
        data_access::ReSampleMethod m
    ) override;

    const GridMask& mask() const noexcept;

  private:
    int               var_grid_id_;
    GridSpecification var_grid_;
    GridMask          var_grid_mask_;
};

} // namespace data_access

#endif // NGEN_WITH_PYTHON
