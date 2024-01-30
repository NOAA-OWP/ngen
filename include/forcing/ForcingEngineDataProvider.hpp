#ifndef NGEN_FORCING_FORCINGENGINE_DATA_PROVIDER_HPP
#define NGEN_FORCING_FORCINGENGINE_DATA_PROVIDER_HPP
#include <NGenConfig.h>

static_assert(
    ngen::exec_info::with_python,
    "ForcingEngineDataProvider requires Python support."
);

#include "GenericDataProvider.hpp"
#include "bmi/Bmi_Py_Adapter.hpp"

namespace data_access {

struct ForcingEngineDataProvider
  : public GenericDataProvider
{
    explicit ForcingEngineDataProvider(const std::string& init);

    ~ForcingEngineDataProvider() override;

    auto get_available_variable_names()
      -> boost::span<const std::string> override;

    auto get_data_start_time()
      -> long override;

    auto get_data_stop_time()
      -> long override;

    auto record_duration()
      -> long override;

    auto get_ts_index_for_time(const time_t& epoch_time)
      -> size_t override;

    auto get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m)
      -> double override;

    auto get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
      -> std::vector<double> override;

  private:
    models::bmi::Bmi_Py_Adapter instance_;
    std::vector<std::string> outputs_;
};

} // namespace data_access

#endif // NGEN_FORCING_FORCINGENGINE_DATA_PROVIDER_HPP
