#include "ForcingsEngineDataProvider.hpp"

#include <boost/core/span.hpp>
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

ForcingsEngineDataProvider::ForcingsEngineDataProvider(const std::string& init, std::size_t time_start, std::size_t time_end)
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

ForcingsEngineDataProvider::ForcingsEngineDataProvider(const std::string& init, const std::string& time_start, const std::string& time_end, const std::string& fmt)
  : ForcingsEngineDataProvider(init, parse_time(time_start, fmt), parse_time(time_end, fmt))
{};

ForcingsEngineDataProvider::~ForcingsEngineDataProvider() = default;

auto ForcingsEngineDataProvider::get_available_variable_names()
  -> boost::span<const std::string>
{
    return outputs_;
}

auto ForcingsEngineDataProvider::get_data_start_time()
  -> long
{
    return clock_type::to_time_t(start_);
}

auto ForcingsEngineDataProvider::get_data_stop_time()
  -> long
{
    return clock_type::to_time_t(end_);
}

auto ForcingsEngineDataProvider::record_duration()
  -> long
{
    return static_cast<long>(instance_.GetTimeStep());
}

auto ForcingsEngineDataProvider::get_ts_index_for_time(const time_t& epoch_time)
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

auto ForcingsEngineDataProvider::get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m)
  -> double
{  
    const auto values = get_values(selector, m);

    switch (m) {
      case ReSampleMethod::SUM:
        return std::accumulate(values.begin(), values.end(), 0.0);

      case ReSampleMethod::MEAN:
        return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());

      case ReSampleMethod::FRONT_FILL:
      case ReSampleMethod::BACK_FILL:
      default:
        throw std::runtime_error{"Given ReSampleMethod " + std::to_string(m) + " not implemented."};
    }
}

auto ForcingsEngineDataProvider::get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
  -> std::vector<double>
{
    // Ensure requested variable is an available variable
    std::string var_name;
    const std::string requested_var_name = selector.get_variable_name();
    const boost::span<const std::string> var_names = get_available_variable_names();

    // Find requested variable, or the element variation
    for (const auto& forcing_name : var_names) {
        if (forcing_name == requested_var_name || forcing_name == (requested_var_name + "_ELEMENT")) {
            var_name = forcing_name;
            break;
        }
    }
    // If variable is not found, throw an exception
    if (var_name.empty()) {
        std::string err{"`" + requested_var_name + "` not found in forcing engine outputs. ("};
        for (const auto& name : var_names) {
            err += name + ", ";
        }

        err.pop_back();
        err.pop_back();
        err.push_back(')');
        throw std::runtime_error{err};
    }

    // Get a span over the catchment IDs and find the request ID
    const std::size_t count         = instance_.GetVarNbytes("CAT-ID") / instance_.GetVarItemsize("CAT-ID");
    const std::string divide_id     = selector.get_id();
    const auto        divide_id_sep = divide_id.find('-');
    const char*       divide_id_pos = divide_id_sep == std::string::npos ? divide_id.data() : &divide_id[divide_id_sep + 1];
    const int         divide_int_id = std::atoi(divide_id_pos);

    // Update forcings engine to the request time index
    const auto start_time  = clock_type::from_time_t(selector.get_init_time());
    const auto duration    = std::chrono::seconds{selector.get_duration_secs()};
    const auto start_index = get_ts_index_for_time(clock_type::to_time_t(start_time));
    const auto end_index   = get_ts_index_for_time(clock_type::to_time_t(start_time + duration));

    std::vector<double> values;
    values.reserve(end_index - start_index);
    for (auto i = start_index; i < end_index; ++i) {
        // TODO: Is this off-by-one? Forcings engine output suggests no, but might need additional unit testing
        instance_.UpdateUntil((start_index + 1) * duration.count());

        // Since catchment IDs are ordered (ascending), we can use binary search
        const auto  ids = boost::span<const int>{static_cast<int*>(instance_.GetValuePtr("CAT-ID")), count};
        const auto* pos = std::lower_bound(ids.cbegin(), ids.cend(), divide_int_id);
        if (pos == std::end(ids) || *pos != divide_int_id) {
            throw std::runtime_error("Failed to find divide ID: `" + std::to_string(divide_int_id) + "`");
        }

        // Get the value based on the index of the requested ID
        const auto result_index = static_cast<int>(std::distance(ids.cbegin(), pos));
        const auto values_span = boost::span<const double>{
          static_cast<double*>(instance_.GetValuePtr(var_name)),
          static_cast<std::size_t>(instance_.GetVarNbytes(var_name) / instance_.GetVarItemsize(var_name))
        };

        values.push_back(values_span[result_index]);
    }

    return values;
}

} // namespace data_access
