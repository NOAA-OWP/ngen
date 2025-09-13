#include "DataProvider.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>  // for std::put_time
#include <forcing/ForcingsEngineLumpedDataProvider.hpp>
#include <iostream>

namespace data_access {

using Provider     = ForcingsEngineLumpedDataProvider;
using BaseProvider = Provider::base_type;

std::size_t convert_divide_id_stoi(const std::string& divide_id)
{
    auto separator = divide_id.find('-');
    const char* split = (
        separator == std::string::npos
        ? &divide_id[0]
        : &divide_id[separator + 1]
    );

    std::stringstream ss; 
    ss.str(""); 
    ss << "Converting divide ID: " << divide_id
       << " -> " << split << std::endl;
    LOG(ss.str(), LogLevel::DEBUG);
    return std::atol(split);
}

Provider::ForcingsEngineLumpedDataProvider(
    const std::string& init_config,
    std::size_t time_begin_seconds,
    std::size_t time_end_seconds,
    const std::string& divide_id
)
  : BaseProvider(init_config, time_begin_seconds, time_end_seconds)
{
    // Add detailed logging of the constructor arguments
    std::stringstream ss; 
    ss.str(""); 
    ss << "Initializing ForcingsEngineLumpedDataProvider:" << std::endl;
    ss << "  init_config:  " << init_config << std::endl;
    LOG(ss.str(), LogLevel::DEBUG);
    
    std::time_t tb_t = static_cast<std::time_t>(time_begin_seconds);
    std::time_t te_t = static_cast<std::time_t>(time_end_seconds);

    ss.str(""); 
    ss << "  Time begin:   " << std::put_time(std::gmtime(&tb_t), "%Y-%m-%d %H:%M:%S UTC")
       << " (" << time_begin_seconds << ")" << std::endl;
    LOG(ss.str(), LogLevel::DEBUG);

    ss.str(""); 
    ss << "  Time end:     " << std::put_time(std::gmtime(&te_t), "%Y-%m-%d %H:%M:%S UTC")
       << " (" << time_end_seconds << ")" << std::endl;
    LOG(ss.str(), LogLevel::DEBUG);

    ss.str(""); 
    ss << "  divide ID:    " << divide_id << std::endl;
    LOG(ss.str(), LogLevel::DEBUG);

    divide_id_ = convert_divide_id_stoi(divide_id);

    // Check that CAT-ID is an available output name, otherwise we most likely aren't
    // running the correct configuration of the forcings engine for this class.
    const auto cat_id_pos = std::find(var_output_names_.begin(), var_output_names_.end(), "CAT-ID");
    if (cat_id_pos == var_output_names_.end()) {

        ss << "Failed to initialize ForcingsEngineLumpedDataProvider: "
                  << "`CAT-ID` is not an output variable of the forcings engine."
                  << " Does " << init_config << " have `GRID_TYPE` set to 'hydrofabric'?" << std::endl;
        throw std::runtime_error{
            "Failed to initialize ForcingsEngineLumpedDataProvider: `CAT-ID` is not an output variable of the forcings engine."
        };
    }
    ss.str(""); 
    ss << " Found CAT-ID in output names" << std::endl;
    LOG(ss.str(), LogLevel::DEBUG);

    var_output_names_.erase(cat_id_pos);

    const auto size_id_dimension = static_cast<std::size_t>(
        bmi_->GetVarNbytes("CAT-ID") / bmi_->GetVarItemsize("CAT-ID")
    );
    ss.str(""); 
    ss << " CAT-ID size: " << size_id_dimension << std::endl;
    LOG(ss.str(), LogLevel::DEBUG);

    // Copy CAT-ID values into instance vector
    const auto cat_id_span = boost::span<const int>(
        static_cast<const int*>(bmi_->GetValuePtr("CAT-ID")),
        size_id_dimension
    );

    auto divide_id_pos = std::find(cat_id_span.begin(), cat_id_span.end(), divide_id_);
    if (divide_id_pos == cat_id_span.end()) {
        ss.str(""); 
        ss << "Unable to find divide ID `" << divide_id
            << "` in the given Forcings Engine domain" << std::endl;
        LOG(ss.str(), LogLevel::SEVERE);
        divide_idx_ = static_cast<std::size_t>(-1);
    } else {
        divide_idx_ = std::distance(cat_id_span.begin(), divide_id_pos);
        ss.str(""); 
        ss << " Divide ID found at index: " << divide_idx_ << std::endl;
        LOG(ss.str(), LogLevel::INFO);
    }

    ss.str(""); 
    ss << " ForcingsEngineLumpedDataProvider initialization complete" << std::endl;
    LOG(LogLevel::DEBUG, ss.str());
}

std::size_t Provider::divide() const noexcept
{
    return divide_id_;
}

std::size_t Provider::divide_index() const noexcept
{
    return divide_idx_;
}

Provider::data_type Provider::get_value(
    const Provider::selection_type& selector,
    data_access::ReSampleMethod m
)
{
    std::stringstream ss; 
    if (!(divide_id_ == convert_divide_id_stoi(selector.get_id()))) {
        ss.str("");
        ss << "get_value() divide_id_ " << divide_id_ << " != selector id " << convert_divide_id_stoi(selector.get_id());
        LOG(LogLevel::FATAL, ss.str());
        throw std::runtime_error{
            "Divide ID mismatch in the forcings engine."
        };
    }

    auto variable = ensure_variable(selector.get_variable_name());

    if (m == ReSampleMethod::SUM || m == ReSampleMethod::MEAN) {
        double acc = 0.0;
        const auto start = clock_type::from_time_t(selector.get_init_time());
        const auto end   = std::chrono::seconds{selector.get_duration_secs()} + start;

        auto tb_t = std::chrono::system_clock::to_time_t(time_begin_);
        auto te_t = std::chrono::system_clock::to_time_t(time_end_);
        auto s_t  = std::chrono::system_clock::to_time_t(start);
        auto e_t  = std::chrono::system_clock::to_time_t(end);

        ss.str(""); 
        ss << "get_value() Time begin: " << std::put_time(std::gmtime(&tb_t), "%Y-%m-%d %H:%M:%S UTC")
            << " (" << tb_t << ")"
            << " | start: " << std::put_time(std::gmtime(&s_t), "%Y-%m-%d %H:%M:%S UTC")
            << " (" << s_t << ")" << std::endl;
        LOG(LogLevel::INFO, ss.str());

        ss.str(""); 
        ss << "get_value() Time end:   " << std::put_time(std::gmtime(&te_t), "%Y-%m-%d %H:%M:%S UTC")
            << " (" << te_t << ")"
            << " | end:   " << std::put_time(std::gmtime(&e_t), "%Y-%m-%d %H:%M:%S UTC")
            << " (" << e_t << ")" << std::endl;
        LOG(LogLevel::INFO, ss.str());

        using namespace std::literals::chrono_literals;
        if (!(start >= time_begin_)) {
            LOG(LogLevel::FATAL,
                "get_value() Begin time less than start: start=%lld vs time_begin_=%lld",
                static_cast<long long>(start.time_since_epoch().count()),
                static_cast<long long>(time_begin_.time_since_epoch().count()));
            throw std::runtime_error{
                "Begin time range error."
            };
        }
        if (!(end < time_end_ + time_step_ + 1s)) {
            LOG(LogLevel::FATAL,
                "get_value() Next Timestep > end: end=%lld vs limit=%lld",
                static_cast<long long>(end.time_since_epoch().count()),
                static_cast<long long>((time_end_ + time_step_ + 1s).time_since_epoch().count()));
            throw std::runtime_error{
                "End time range error."
            };
        }

        auto current = start;
        while (current < end) {
            current += time_step_;
            bmi_->UpdateUntil(std::chrono::duration_cast<std::chrono::seconds>(current - time_begin_).count());
            acc += static_cast<double*>(bmi_->GetValuePtr(variable))[divide_idx_];
        }

        if (m == ReSampleMethod::MEAN) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(current - start).count();
            auto num_time_steps = duration / time_step_.count();
            acc /= num_time_steps;
        }

        return acc;
    }

    ss.str(""); 
    ss << "Given ReSampleMethod " + std::to_string(m) + " not implemented.";
    LOG(LogLevel::FATAL, ss.str());
    throw std::runtime_error{ss.str()};
}

std::vector<Provider::data_type> Provider::get_values(
    const Provider::selection_type& selector,
    data_access::ReSampleMethod /* unused */
)
{
    std::stringstream ss; 
    if (!(divide_id_ == convert_divide_id_stoi(selector.get_id()))) {
        ss.str("");
        ss << "get_values() divide id " << divide_id_ << "not equal to selector.get_id" << convert_divide_id_stoi(selector.get_id());
        LOG(LogLevel::FATAL, ss.str());
        throw std::runtime_error{
            "Divide ID mismatch in the forcings engine."
        };
   }
 
    auto variable = ensure_variable(selector.get_variable_name());

    const auto start = clock_type::from_time_t(selector.get_init_time());
    const auto end = std::chrono::seconds{selector.get_duration_secs()} + start;

    auto tb_t = std::chrono::system_clock::to_time_t(time_begin_);
    auto te_t = std::chrono::system_clock::to_time_t(time_end_);
    auto s_t  = std::chrono::system_clock::to_time_t(start);
    auto e_t  = std::chrono::system_clock::to_time_t(end);

    ss.str("");
    ss << "get_values() Time begin: " << std::put_time(std::gmtime(&tb_t), "%Y-%m-%d %H:%M:%S UTC")
              << " (" << tb_t << ")"
              << " | start: " << std::put_time(std::gmtime(&s_t), "%Y-%m-%d %H:%M:%S UTC")
              << " (" << s_t << ")" << std::endl;
    LOG(LogLevel::INFO, ss.str());

    ss.str("");
    ss << "get_values() Time end:   " << std::put_time(std::gmtime(&te_t), "%Y-%m-%d %H:%M:%S UTC")
              << " (" << te_t << ")"
              << " | end:   " << std::put_time(std::gmtime(&e_t), "%Y-%m-%d %H:%M:%S UTC")
              << " (" << e_t << ")" << std::endl;
    LOG(LogLevel::INFO, ss.str());

    using namespace std::literals::chrono_literals;
    if (!(start >= time_begin_)) {
        LOG(LogLevel::FATAL,
            "get_values() Begin time less than start: start=%lld vs time_begin_=%lld",
            static_cast<long long>(start.time_since_epoch().count()),
            static_cast<long long>(time_begin_.time_since_epoch().count()));
        throw std::runtime_error{
            "Start time range error."
        };
    }
    if (!(end < time_end_ + time_step_ + 1s)) {
        LOG(LogLevel::FATAL,
            "get_values() Next Timestep > end: end=%lld vs limit=%lld",
            static_cast<long long>(end.time_since_epoch().count()),
            static_cast<long long>((time_end_ + time_step_ + 1s).time_since_epoch().count()));
        throw std::runtime_error{
            "End time range error."
        };
    }   

    
    std::vector<double> values;
    auto current = start;
    while (current < end) {
        current += time_step_;
        bmi_->UpdateUntil(std::chrono::duration_cast<std::chrono::seconds>(current - time_begin_).count());
        values.push_back(static_cast<double*>(bmi_->GetValuePtr(variable))[divide_idx_]);
    }

    return values;
}

} // namespace data_access
