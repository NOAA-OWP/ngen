#pragma once

#include "GridDataSelector.hpp"
#include "ForcingsEngineDataProvider.hpp"

#include <utilities/mdarray.hpp>

namespace data_access {

struct ForcingsEngineGriddedDataProvider
  : public ForcingsEngineDataProvider<Cell, GridDataSelector>
{
    ~ForcingsEngineGriddedDataProvider() override = default;

    Cell get_value(const GridDataSelector& selector, data_access::ReSampleMethod m) override;

    std::vector<Cell> get_values(const GridDataSelector& selector, data_access::ReSampleMethod m) override;

    static ForcingsEngineDataProvider* make_gridded_instance(
        const std::string& init,
        const std::string& time_start,
        const std::string& time_end,
        const std::string& time_fmt = default_time_format
    )
    {
        return make_instance<ForcingsEngineGriddedDataProvider>(init, time_start, time_end, time_fmt);
    }

  private:
    ForcingsEngineGriddedDataProvider(
        const std::string& init,
        std::size_t time_begin_seconds,
        std::size_t time_end_seconds
    );

    int var_grid_id_{};
    GridSpecification var_grid_{};
};

} // namespace data_access
