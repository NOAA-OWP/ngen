#include "ForcingEngineDataProvider.hpp"

#include <boost/core/span.hpp>
#include <boost/utility/string_view.hpp>
#include <stdexcept>

namespace data_access {

auto parse_time(const std::string& time, const std::string& fmt)
  -> time_t
{
    std::tm tm_ = {};
    std::stringstream{time} >> std::get_time(&tm_, fmt.c_str());

    // Note: `timegm` is available for Linux and BSD (aka macOS) via time.h, but not Windows.
    return timegm(&tm_);
}

ForcingEngineDataProvider::ForcingEngineDataProvider(const std::string& init, std::size_t time_start, std::size_t time_end)
  : instance_(
      "ForcingEngine",
      init,
      "NextGen_Forcings_Engine.BMIForcingsEngine",
      true,
      true,
      utils::getStdOut()
    )
  , start_(std::chrono::seconds(time_start))
  , end_(std::chrono::seconds(time_end))
  , outputs_(instance_.GetOutputVarNames())
{
    instance_.Initialize();
};

ForcingEngineDataProvider::ForcingEngineDataProvider(const std::string& init, const std::string& time_start, const std::string& time_end, const std::string& fmt)
  : ForcingEngineDataProvider(init, parse_time(time_start, fmt), parse_time(time_end, fmt))
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
    return clock_type::to_time_t(start_);
}

auto ForcingEngineDataProvider::get_data_stop_time()
  -> long
{
    return clock_type::to_time_t(end_);
}

auto ForcingEngineDataProvider::record_duration()
  -> long
{
    // FIXME: Temporary, but most likely incorrect, cast
    return instance_.GetTimeStep();
}

auto ForcingEngineDataProvider::get_ts_index_for_time(const time_t& epoch_time)
  -> size_t
{
    const auto epoch = clock_type::from_time_t(epoch_time);

    if (epoch < start_ || epoch > end_) {
        auto start = clock_type::to_time_t(start_);
        auto end = clock_type::to_time_t(end_);
        throw std::runtime_error{
          "Epoch " + std::to_string(epoch_time) + " is not within the interval " +
          "[" +
            std::ctime(&start) + " (" + std::to_string(start) + "), " +
            std::ctime(&end) + " (" + std::to_string(end)  + ")" +
          "]"
        };
    }

    return std::chrono::duration_cast<std::chrono::seconds>(epoch - start_).count() / record_duration();
}

auto ForcingEngineDataProvider::get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m)
  -> double
{
    const std::string var_name  = selector.get_variable_name();
    const auto var_names = get_available_variable_names();
    if (std::find(var_names.begin(), var_names.end(), var_name) == var_names.end()) {
        std::string err{"`" + var_name + "` not found in forcing engine outputs. ("};
        for (const auto& name : var_names) {
            err += name + ", ";
        }
        err.pop_back();
        err.pop_back();
        err.push_back(')');
        throw std::runtime_error{err};
    }

    const std::string divide_id = selector.get_id();

    const auto start_time = clock_type::from_time_t(selector.get_init_time());
    const auto duration   = std::chrono::seconds{selector.get_duration_secs()};
    const auto start_index = get_ts_index_for_time(clock_type::to_time_t(start_time));
    const auto end_index = get_ts_index_for_time(clock_type::to_time_t(start_time + duration));
    for (auto i = start_index; i < end_index; ++i) {
        std::cout << "Updating to: " << i << '\n';
        instance_.Update();
    }
    std::cout << "Finished updating" << '\n';
    const std::size_t count     = instance_.GetVarNbytes("CAT-ID") / instance_.GetVarItemsize("CAT-ID");
    const auto divide_int_id    = std::atoi(divide_id.data() + divide_id.find('-') + 1);
    const auto ids              = boost::span<const int>{static_cast<int*>(instance_.GetValuePtr("CAT-ID")), count};
    
    std::cout << "Searching for ID " << std::to_string(divide_int_id) << '\n';
    const auto* pos = std::lower_bound(ids.cbegin(), ids.cend(), divide_int_id);
    if (pos == std::end(ids) || *pos != divide_int_id) {
        throw std::runtime_error("Failed to get variable");
    }

    const auto result_index = static_cast<int>(std::distance(ids.cbegin(), pos));
    std::cout << "Found ID at index " << result_index << '\n';
  
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
