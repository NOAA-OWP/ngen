#pragma once

#include <NGenConfig.h>

#if NGEN_WITH_PYTHON

#include <forcing/ForcingsEngineDataProvider.hpp>
#include <forcing/DataProviderSelectors.hpp>

namespace data_access {

struct ForcingsEngineLumpedDataProvider final :
  public ForcingsEngineDataProvider<double, CatchmentAggrDataSelector>
{
    using base_type = ForcingsEngineDataProvider<data_type, selection_type>;

    ~ForcingsEngineLumpedDataProvider() override = default;

    ForcingsEngineLumpedDataProvider(
        const std::string& init,
        std::size_t time_begin_seconds,
        std::size_t time_end_seconds,
        const std::string& divide_id
    );

    data_type get_value(
        const selection_type& selector,
        data_access::ReSampleMethod m
    ) override;

    std::vector<data_type> get_values(
        const selection_type& selector,
        data_access::ReSampleMethod m
    ) override;

    //! Get this provider's Divide ID.
    std::size_t divide() const noexcept;

    //! Get this provider's Divide ID index within the Forcings Engine.
    std::size_t divide_index() const noexcept;

  private:
    std::size_t divide_id_;
    std::size_t divide_idx_;
};

} // namespace data_access

#endif // NGEN_WITH_PYTHON
