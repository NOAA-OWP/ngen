#include <chrono>
#include <forcing/ForcingsEngineGriddedDataProvider.hpp>

namespace data_access {

using Provider     = ForcingsEngineGriddedDataProvider;
using BaseProvider = Provider::base_type;

GridSpecification construct_grid_spec(bmi::Bmi* ptr, int grid)
{
    const auto grid_type =  ptr->GetGridType(grid);
    assert(grid_type == "uniform_rectilinear");

    std::vector<double> coords;
    std::array<int, 2> shape = {-1, -1};
    ptr->GetGridShape(grid, shape.data());
    assert(shape[0] > 0);
    assert(shape[1] > 0);

    std::cout << "Grid Shape: " << shape[0] << ", " << shape[1] << '\n';

    // Get X bounds
    coords.resize(shape[1]);
    ptr->GetGridX(grid, coords.data());
    auto xminmax = std::minmax_element(coords.begin(), coords.end());
    double xmin  = *xminmax.first;
    double xmax  = *xminmax.second;

    // Get Y bounds
    coords.resize(shape[0]);
    ptr->GetGridY(grid, coords.data());
    auto yminmax = std::minmax_element(coords.begin(), coords.end());
    double ymin  = *yminmax.first;
    double ymax  = *yminmax.second;

    return {
        /*rows=*/static_cast<std::uint64_t>(shape[0]),
        /*columns=*/static_cast<std::uint64_t>(shape[1]),
        /*extent=*/{
            /*min_corner=*/{xmin, ymin},
            /*max_corner=*/{xmax, ymax}
        }
    };
}

template<std::size_t I>
std::uint64_t position_helper(
    const geojson::coordinate_t& mask,
    const GridSpecification& grid
)
{
    return GridMask::position(
        mask.get<I>(),
        grid.extent.min_corner().get<I>(),
        grid.extent.max_corner().get<I>(),
        I == 0 /*x*/ ? grid.columns : grid.rows
    );
}

GridMask construct_grid_mask(geojson::box_t mask_extent, const GridSpecification& underlying_grid)
{
    auto cmin = position_helper<0>(mask_extent.min_corner(), underlying_grid);
    auto cmax = position_helper<0>(mask_extent.max_corner(), underlying_grid);
    auto rmin = position_helper<1>(mask_extent.min_corner(), underlying_grid);
    auto rmax = position_helper<1>(mask_extent.max_corner(), underlying_grid);

    return { std::move(mask_extent), rmin, cmin, rmax, cmax };
}

Provider::ForcingsEngineGriddedDataProvider(
    const std::string& init,
    std::size_t time_begin_seconds,
    std::size_t time_end_seconds,
    geojson::box_t mask
)
  : BaseProvider(init, time_begin_seconds, time_end_seconds)
{
    // FIXME: assert that var_grid_mask_ is (entirely) within var_grid_
    //        (possibly, convert to polygon and use contains predicate)
    // NOTE: take only first variable name because all variables share the same grid
    //       in the forcings engine.
    var_grid_id_   = bmi_->GetVarGrid(get_available_variable_names()[0]);
    var_grid_      = construct_grid_spec(bmi_.get(), var_grid_id_);
    var_grid_mask_ = construct_grid_mask(std::move(mask), var_grid_);
}

Provider::data_type Provider::get_value(const selection_type&, data_access::ReSampleMethod)
{
    throw std::runtime_error{"ForcingsEngineGriddedDataProvider::get_value() is not implemented"};
}

std::vector<Provider::data_type> Provider::get_values(
    const selection_type& selector,
    data_access::ReSampleMethod m
)
{
    auto variable = ensure_variable(selector.variable_name);

    if (m != ReSampleMethod::SUM && m != ReSampleMethod::MEAN) {
        throw std::runtime_error{
            "ForcingsEngineGriddedDataProvider::get_values(): " 
            "ReSampleMethod " + std::to_string(m) + " not implemented."};
    }

    const auto duration = std::chrono::seconds{selector.duration};
    const auto start    = clock_type::from_time_t(selector.init_time);
    assert(start >= time_begin_);

    auto until = (start - time_begin_) + duration;
    if (until > time_end_ - time_begin_) {
        until = time_end_ - time_begin_;
    }

    std::vector<double> values;
    values.resize(var_grid_mask_.size());
    while (std::chrono::seconds{std::lround(bmi_->GetCurrentTime())} < until) {
        // Get a span over the entire grid
        boost::span<const double> full = { static_cast<double*>(bmi_->GetValuePtr(variable)), var_grid_.rows * var_grid_.columns };

        // Iterate row by row over the grid, masking the grid columns in each row.
        // For each row, we add the grid values to the masked grid values.
        for (auto r = var_grid_mask_.rmin; r < var_grid_mask_.rmax; ++r) {
            // Get the starting index of the current row within the full span
            // Equation: <starting column offset> + (<row offset> * <row size>)
            const std::size_t row_address = var_grid_mask_.cmin + (r * var_grid_.columns);

            // Get the starting index (pointer address) of the current row within the masked grid
            // Equation: <data pointer> + (<row offset> * <row size>)
            double* const mask_address = values.data() + ((r - var_grid_mask_.rmin) * var_grid_mask_.columns());

            // Get a span over the current row index on the underlying grid
            boost::span<const double> row = full.subspan(row_address, var_grid_mask_.columns());

            // Print Row/Column Values
            // std::cout << "row " << r << ": ";
            // for (auto c = 0; c < var_grid_mask_.columns(); ++c) {
            //     std::cout << row[c] << ' ';
            // }
            // std::cout << '\n';
            
            // Get a mutable span over the current row index in the masked values
            boost::span<double> masked = { mask_address, var_grid_mask_.columns() };

            // Add grid values to masked values
            std::transform(row.begin(), row.end(), masked.begin(), masked.begin(), std::plus<double>{});
        }

        bmi_->Update();
    }

    if (m == ReSampleMethod::MEAN) {
        auto steps = duration / time_step_;
        for (auto& v : values) {
            v /= steps;
        }
    }

    return values;
}

const GridMask& Provider::mask() const noexcept
{
    return var_grid_mask_;
}

} // namespace data_access
