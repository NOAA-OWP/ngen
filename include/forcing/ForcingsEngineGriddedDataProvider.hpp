#pragma once

#include "GridDataSelector.hpp"
#include "ForcingsEngineDataProvider.hpp"

#include <utilities/mdarray.hpp>

namespace data_access {

struct ForcingsEngineGriddedDataProvider
  : public ForcingsEngineDataProvider<Cell, GridDataSelector>
{
    using data_type = data_type;
    using selection_type = selection_type;
    using base_type = ForcingsEngineDataProvider<data_type, selection_type>;

    ~ForcingsEngineGriddedDataProvider() override = default;

    data_type get_value(const selection_type& selector, data_access::ReSampleMethod m) override;

    std::vector<data_type> get_values(const selection_type& selector, data_access::ReSampleMethod m) override;

    static std::unique_ptr<ForcingsEngineDataProvider> make_gridded_instance(
        const std::string& init,
        const std::string& time_start,
        const std::string& time_end,
        const std::string& time_fmt = default_time_format
    )
    {
        return make_instance<ForcingsEngineGriddedDataProvider>(init, time_start, time_end, time_fmt);
    }

  private:
    friend base_type;

    ForcingsEngineGriddedDataProvider(
        const std::string& init,
        std::time_t time_begin_seconds,
        std::time_t time_end_seconds
    );

    int var_grid_id_{};
    GridSpecification var_grid_{};
};

} // namespace data_access
