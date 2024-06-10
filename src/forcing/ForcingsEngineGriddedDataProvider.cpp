#include "ForcingsEngineGriddedDataProvider.hpp"
#include "DataProvider.hpp"
#include "GridDataSelector.hpp"

namespace data_access {

using BaseProvider = ForcingsEngineDataProvider<Cell, GridDataSelector>;
using Provider = ForcingsEngineGriddedDataProvider;

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
    var_grid_id_ = bmi_->GetVarGrid(get_available_variable_names()[0]);
    std::array<int, 2> shape = {-1, -1};
    bmi_->GetGridShape(var_grid_id_, shape.data());
    assert(shape[0] == shape[1]); // since the grid must be uniform rectilinear

    // Allocate a coordinate vector and reuse for X and Y
    std::vector<double> coordinate(shape[0]);

    // Get longitude bounds
    bmi_->GetGridX(var_grid_id_, coordinate.data());
    auto xminmax = std::minmax_element(coordinate.begin(), coordinate.end());
    double xmin = *xminmax.first;
    double xmax = *xminmax.second;

    // Get latitude bounds
    bmi_->GetGridY(var_grid_id_, coordinate.data());
    auto yminmax = std::minmax_element(coordinate.begin(), coordinate.end());
    double ymin = *yminmax.first;
    double ymax = *yminmax.second;

    // Construct grid specification
    var_grid_ = {
        static_cast<std::uint64_t>(shape[0]),
        static_cast<std::uint64_t>(shape[1]),
        box_t{{ xmin, ymin }, { xmax, ymax}}
    };
}

Cell Provider::get_value(const GridDataSelector& selector, data_access::ReSampleMethod m)
{
    if (m != ReSampleMethod::SUM && m != ReSampleMethod::MEAN) {
        throw std::runtime_error{"Given ReSampleMethod " + std::to_string(m) + " not implemented."};
    }

    const auto start = clock_type::from_time_t(selector.initial_time());
    const auto end = std::chrono::seconds{selector.duration()} + start;
    const auto step = std::chrono::seconds{record_duration()};

    auto cell = selector.cells()[0]; // FIXME: bad semantics, 
    
    for (auto current = start; current < end; current += step) {
        bmi_->UpdateUntil(current.time_since_epoch().count());

        boost::span<double> values{
            static_cast<double*>(bmi_->GetValuePtr(selector.variable())),
            var_grid_.rows * var_grid_.columns
        };

        cell.value += values[cell.x + cell.y * var_grid_.rows];
    }

    if (m == ReSampleMethod::MEAN) {
        const auto time_step_seconds = step.count();
        const auto time_duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        const auto num_time_steps = time_duration / time_step_seconds;
        cell.value /= num_time_steps;
    }

    return cell;
}

std::vector<Cell> Provider::get_values(const GridDataSelector& selector, data_access::ReSampleMethod m)
{
    
}


} // namespace data_access
