#include "ForcingsEngineDataProvider.hpp"

namespace data_access {

auto parse_time(const std::string& time, const std::string& fmt)
  -> time_t
{
    std::tm tm_ = {};
    std::stringstream{time} >> std::get_time(&tm_, fmt.c_str());

    // Note: `timegm` is available for Linux and BSD (aka macOS) via time.h, but not Windows.
    return timegm(&tm_);
}

ForcingsEngineDataProvider::ForcingsEngineDataProvider(
    const std::string& init,
    std::size_t time_start,
    std::size_t time_end
) : engine_(
    &ForcingsEngine::instance(init, time_start, time_end)
){}

ForcingsEngineDataProvider::ForcingsEngineDataProvider(
    const std::string& init,
    const std::string& time_start,
    const std::string& time_end,
    const std::string& time_fmt
) : ForcingsEngineDataProvider(
    init,
    parse_time(time_start, time_fmt),
    parse_time(time_end, time_fmt)
){}

ForcingsEngineDataProvider::~ForcingsEngineDataProvider() = default;


auto ForcingsEngineDataProvider::get_available_variable_names()
  -> boost::span<const std::string>
{
    return engine_->outputs();
}

auto ForcingsEngineDataProvider::get_data_start_time()
  -> long
{
    return ForcingsEngine::clock_type::to_time_t(engine_->time_begin());
}

auto ForcingsEngineDataProvider::get_data_stop_time()
  -> long
{
    return ForcingsEngine::clock_type::to_time_t(engine_->time_end());
}

auto ForcingsEngineDataProvider::record_duration()
  -> long
{
    return engine_->time_step().count();
}

auto ForcingsEngineDataProvider::get_ts_index_for_time(const time_t& epoch_time)
  -> size_t
{
    return engine_->time_index(epoch_time);
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

auto ForcingsEngineDataProvider::get_values(const CatchmentAggrDataSelector& selector, ReSampleMethod m)
  -> std::vector<double>
{
    const auto start = ForcingsEngine::clock_type::from_time_t(selector.get_init_time());
    const auto end   = std::chrono::seconds{selector.get_duration_secs()} + start;

    std::vector<double> values;
    for (auto current_time = start; current_time < end; current_time += engine_->time_step()) {
        
        values.push_back(
            engine_->at(current_time, selector.get_id(), selector.get_variable_name())
        );
    }

    return values;
}

} // namespace data_access
