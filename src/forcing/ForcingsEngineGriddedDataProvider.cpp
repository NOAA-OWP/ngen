#include "ForcingsEngineGriddedDataProvider.hpp"
#include "DataProvider.hpp"
#include "GridDataSelector.hpp"

namespace data_access {

using Provider = ForcingsEngineGriddedDataProvider;
using BaseProvider = Provider::base_type;

// The following grid specification is a specific optimization
// for the NextGen Forcings Engine, since in gridded mode, all
// variables share the same grid.
GridSpecification construct_fe_grid_spec(bmi::Bmi* ptr, int grid)
{
    std::array<int, 2> shape = {-1, -1};
    ptr->GetGridShape(grid, shape.data());
    assert(shape[0] == shape[1]); // since the grid must be uniform rectilinear

    // Allocate a coordinate vector and reuse for X and Y
    std::vector<double> coordinate(shape[0]);

    // Get longitude bounds
    ptr->GetGridX(grid, coordinate.data());
    auto xminmax = std::minmax_element(coordinate.begin(), coordinate.end());
    double xmin = *xminmax.first;
    double xmax = *xminmax.second;

    // Get latitude bounds
    ptr->GetGridY(grid, coordinate.data());
    auto yminmax = std::minmax_element(coordinate.begin(), coordinate.end());
    double ymin = *yminmax.first;
    double ymax = *yminmax.second;

    return {
        static_cast<std::uint64_t>(shape[0]),
        static_cast<std::uint64_t>(shape[1]),
        BoundingBox{xmin, xmax, ymin, ymax}
    };
}

Provider::ForcingsEngineGriddedDataProvider(
    const std::string& init,
    std::size_t time_begin_seconds,
    std::size_t time_end_seconds
)
  : BaseProvider(init, time_begin_seconds, time_end_seconds)
{
    // The following grid specification is a specific optimization
    // for the NextGen Forcings Engine, since in gridded mode, all
    // variables share the same grid.
    var_grid_id_   = bmi_->GetVarGrid(get_available_variable_names()[0]);
    var_grid_      = construct_fe_grid_spec(bmi_.get(), var_grid_id_);
    var_grid_mask_ = var_grid_.extent;
}

Provider::ForcingsEngineGriddedDataProvider(
    const std::string& init,
    std::size_t time_begin_seconds,
    std::size_t time_end_seconds,
    const BoundingBox& mask
) : ForcingsEngineGriddedDataProvider(
        init,
        time_begin_seconds,
        time_end_seconds
  )
{
    // TODO: assert mask is contained within the grid spec extent
    var_grid_mask_ = mask;
}

Provider::ForcingsEngineGriddedDataProvider(
    const std::string& init,
    std::size_t time_begin_seconds,
    std::size_t time_end_seconds,
    const geojson::polygon_t& boundary
) : ForcingsEngineGriddedDataProvider(
        init,
        time_begin_seconds,
        time_end_seconds,
        BoundingBox{boundary}
    )
{}

Provider::data_type Provider::get_value(const GridDataSelector& selector, data_access::ReSampleMethod m)
{
    throw std::runtime_error{"ForcingsEngineGriddedDataProvider::get_value() is not implemented"};
}

std::vector<Provider::data_type> Provider::get_values(const GridDataSelector& selector, data_access::ReSampleMethod m)
{
    throw std::runtime_error{"ForcingsEngineGriddedDataProvider::get_values() is not implemented"}; // Temporary

    if (m != ReSampleMethod::SUM && m != ReSampleMethod::MEAN) {
        throw std::runtime_error{"Given ReSampleMethod " + std::to_string(m) + " not implemented."};
    }

    const auto start = clock_type::from_time_t(selector.init_time);
    const auto end = std::chrono::seconds{selector.duration} + start;
    const auto step = std::chrono::seconds{record_duration()};

    // std::vector<double> values;

    // std::vector<Cell> cells = { selector.cells().begin(), selector.cells().end() };
    for (auto current = start; current < end; current += step) {
        this->next(current.time_since_epoch().count());

        boost::span<double> values{
            static_cast<double*>(bmi_->GetValuePtr(selector.variable_name)),
            var_grid_.rows * var_grid_.columns
        };

        // for (auto& cell : cells) {
        //     cell.value += values[cell.x + cell.y * var_grid_.rows];
        // }
    }

    if (m == ReSampleMethod::MEAN) {
        const auto time_step_seconds = step.count();
        const auto time_duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        const auto num_time_steps = time_duration / time_step_seconds;

        // for (auto& cell : cells) {
        //     cell.value /= num_time_steps;
        // }
    }

    // return cells;
    throw;
}


} // namespace data_access
