#include "ForcingsEngineDataProvider.hpp"
#include "DataProvider.hpp"

#include <iomanip>

namespace data_access {

time_t parse_time(const std::string& time, const std::string& fmt)
{
    std::tm tm_ = {};
    std::stringstream tmstr{time};
    tmstr >> std::get_time(&tm_, fmt.c_str());

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


boost::span<const std::string> ForcingsEngineDataProvider::get_available_variable_names()
{
    return engine_->outputs();
}

long ForcingsEngineDataProvider::get_data_start_time()
{
    return ForcingsEngine::clock_type::to_time_t(engine_->time_begin());
}

long ForcingsEngineDataProvider::get_data_stop_time()
{
    return ForcingsEngine::clock_type::to_time_t(engine_->time_end());
}

long ForcingsEngineDataProvider::record_duration()
{
    return engine_->time_step().count();
}

size_t ForcingsEngineDataProvider::get_ts_index_for_time(const time_t& epoch_time)
{
    return engine_->time_index(epoch_time);
}

double ForcingsEngineDataProvider::get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m)
{
    const auto start = ForcingsEngine::clock_type::from_time_t(selector.get_init_time());
    const auto end = std::chrono::seconds{selector.get_duration_secs()} + start;
    const std::string id = selector.get_id();
    const std::string var = selector.get_variable_name();

    if (m == ReSampleMethod::SUM || m == ReSampleMethod::MEAN) {
        double acc = 0.0;
        for (auto current_time = start; current_time < end; current_time += engine_->time_step()) {
            acc += engine_->at(current_time, id, var);
        }

        if (m == ReSampleMethod::MEAN) {
            const auto time_step_seconds = std::chrono::duration_cast<std::chrono::seconds>(engine_->time_step()).count();
            const auto num_time_steps = selector.get_duration_secs() / time_step_seconds;
            acc /= num_time_steps;
        }

        return acc;
    }

    throw std::runtime_error{"Given ReSampleMethod " + std::to_string(m) + " not implemented."};
}

std::vector<double> ForcingsEngineDataProvider::get_values(const CatchmentAggrDataSelector& selector, ReSampleMethod m)
{
    const auto start = ForcingsEngine::clock_type::from_time_t(selector.get_init_time());
    const auto end   = std::chrono::seconds{selector.get_duration_secs()} + start;
    const std::string id = selector.get_id();
    const std::string var = selector.get_variable_name();

    std::vector<double> values;
    for (auto current_time = start; current_time < end; current_time += engine_->time_step()) {
        values.push_back(engine_->at(current_time, id, var));
    }

    return values;
}

} // namespace data_access
