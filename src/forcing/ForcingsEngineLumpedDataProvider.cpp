#include "DataProvider.hpp"
#include <chrono>
#include <forcing/ForcingsEngineLumpedDataProvider.hpp>

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

    return std::atol(split);
}

Provider::ForcingsEngineLumpedDataProvider(
    const std::string& init,
    std::size_t time_begin_seconds,
    std::size_t time_end_seconds,
    const std::string& divide_id
)
  : BaseProvider(init, time_begin_seconds, time_end_seconds)
{
    divide_id_ = convert_divide_id_stoi(divide_id);

    // Check that CAT-ID is an available output name, otherwise we most likely aren't
    // running the correct configuration of the forcings engine for this class.
    const auto cat_id_pos = std::find(var_output_names_.begin(), var_output_names_.end(), "CAT-ID");
    if (cat_id_pos == var_output_names_.end()) {
        throw std::runtime_error{
            "Failed to initialize ForcingsEngineLumpedDataProvider: `CAT-ID` is not an output variable of the forcings engine."
            " Does " + init + " have `GRID_TYPE` set to 'hydrofabric'?"
        };
    }
    var_output_names_.erase(cat_id_pos);

    const auto size_id_dimension = static_cast<std::size_t>(
        bmi_->GetVarNbytes("CAT-ID") / bmi_->GetVarItemsize("CAT-ID")
    );

    // Copy CAT-ID values into instance vector
    const auto cat_id_span = boost::span<const int>(
        static_cast<const int*>(bmi_->GetValuePtr("CAT-ID")),
        size_id_dimension
    );

    auto divide_id_pos = std::find(cat_id_span.begin(), cat_id_span.end(), divide_id_);
    if (divide_id_pos == cat_id_span.end()) {
        // throw std::runtime_error{"Unable to find divide ID `" + divide_id + "` in given Forcings Engine domain"};
        divide_idx_ = static_cast<std::size_t>(-1);
    } else {
        divide_idx_ = std::distance(cat_id_span.begin(), divide_id_pos);
    }
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
    assert(divide_id_ == convert_divide_id_stoi(selector.get_id()));

    auto variable = ensure_variable(selector.get_variable_name());

    if (m == ReSampleMethod::SUM || m == ReSampleMethod::MEAN) {
        double acc = 0.0;
        const auto start = clock_type::from_time_t(selector.get_init_time());
        assert(start >= time_begin_);

        const auto end = std::chrono::seconds{selector.get_duration_secs()} + start;
        assert(end <= time_end_);

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

    throw std::runtime_error{"Given ReSampleMethod " + std::to_string(m) + " not implemented."};
}

std::vector<Provider::data_type> Provider::get_values(
    const Provider::selection_type& selector,
    data_access::ReSampleMethod /* unused */
)
{
    assert(divide_id_ == convert_divide_id_stoi(selector.get_id()));

    auto variable = ensure_variable(selector.get_variable_name());

    const auto start = clock_type::from_time_t(selector.get_init_time());
    assert(start >= time_begin_);

    const auto end = std::chrono::seconds{selector.get_duration_secs()} + start;
    assert(end <= time_end_);

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
