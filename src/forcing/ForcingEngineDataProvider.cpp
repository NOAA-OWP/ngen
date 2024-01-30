#include "ForcingEngineDataProvider.hpp"

#include <boost/core/span.hpp>
#include <boost/utility/string_view.hpp>

namespace data_access {

ForcingEngineDataProvider::ForcingEngineDataProvider(const std::string& init)
  : instance_(
      "ForcingEngine",
      init,
      "NextGen_Forcings_Engine.BMIForcingsEngine",
      true,
      true,
      utils::getStdOut()
    )
  , outputs_(instance_.GetOutputVarNames())
{};

ForcingEngineDataProvider::~ForcingEngineDataProvider() = default;

auto ForcingEngineDataProvider::get_available_variable_names()
  -> boost::span<const std::string>
{
    return outputs_;
}

auto ForcingEngineDataProvider::get_data_start_time()
  -> long
{
    // FIXME: Temporary, but most likely incorrect, cast
    return static_cast<int64_t>(instance_.GetStartTime());
}

auto ForcingEngineDataProvider::get_data_stop_time()
  -> long
{
    // FIXME: Temporary, but most likely incorrect, cast
    return static_cast<int64_t>(instance_.GetEndTime());
}

auto ForcingEngineDataProvider::record_duration()
  -> long
{
    // FIXME: Temporary, but most likely incorrect, cast
    return static_cast<int64_t>(instance_.GetTimeStep());
}

auto ForcingEngineDataProvider::get_ts_index_for_time(const time_t& epoch_time)
  -> size_t
{
    // TODO: implementation
    throw std::runtime_error{"not implemented"};
}

auto ForcingEngineDataProvider::get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m)
  -> double
{
    const std::string var_name  = selector.get_variable_name();
    const std::string divide_id = selector.get_id();
    const std::size_t count     = instance_.GetVarNbytes("CAT-ID") / instance_.GetVarItemsize("CAT-ID");
    const auto divide_int_id    = std::atoi(divide_id.data() + divide_id.find('-'));
    const auto ids              = boost::span<const int>{static_cast<int*>(instance_.GetValuePtr("CAT-ID")), count};
    const auto* pos             = std::find(ids.cbegin(), ids.cend(), divide_int_id);

    if (pos == std::end(ids)) {
        throw std::runtime_error("Failed to get variable");
    }

    const auto result_index = static_cast<int>(std::distance(ids.cbegin(), pos));    
    const auto values = boost::span<const double>{
      static_cast<double*>(instance_.GetValuePtr(var_name)),
      static_cast<std::size_t>(instance_.GetVarNbytes(var_name) / instance_.GetVarItemsize(var_name))
    };

    return values[result_index];
}

auto ForcingEngineDataProvider::get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
  -> std::vector<double>
{
    return {0.0};
}

} // namespace data_access
