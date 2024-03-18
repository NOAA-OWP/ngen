#include "ForcingsEngineDataProvider.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>

#include "Bmi_Py_Adapter.hpp"
#include "mdarray.hpp"

namespace data_access {

std::unique_ptr<ForcingsEngine> ForcingsEngine::inst_{};

ForcingsEngine::ForcingsEngine(const std::string& init, size_type time_start, size_type time_end)
  : time_start_(std::chrono::seconds{time_start})
  , time_end_(std::chrono::seconds{time_end})
{
    check_runtime_requirements();

    bmi_ = std::make_unique<models::bmi::Bmi_Py_Adapter>(
        "ForcingsEngine",
        init,
        "NextGen_Forcings_Engine.BMIForcingsEngine",
        true,
        true,
        utils::getStdOut()
    );
    
    time_step_ = std::chrono::seconds{static_cast<int64_t>(bmi_->GetTimeStep())};
    var_outputs_ = bmi_->GetOutputVarNames();
    var_outputs_.erase(var_outputs_.begin()); // Erase "CAT-ID" from outputs
    const auto time_dim = static_cast<size_type>((time_end_ - time_start_) / time_step_);
    const auto id_dim   = static_cast<size_type>(bmi_->GetVarNbytes("CAT-ID") / bmi_->GetVarItemsize("CAT-ID"));
    const auto var_dim  = var_outputs_.size();

    bmi_->Update();
    time_current_index_++;
    
    const auto* ptr = static_cast<int*>(bmi_->GetValuePtr("CAT-ID"));
    var_divides_ = std::vector<int>(ptr, ptr + id_dim);
    var_cache_ = decltype(var_cache_){{ time_dim, id_dim, var_dim }};

    // Cache initial iteration
    const auto size = var_divides_.size();
    for (size_type vi = 0; vi < var_outputs_.size(); vi++) {
        // Get current values for each variable
        const auto var_span = boost::span<const double>{
            static_cast<double*>(bmi_->GetValuePtr(var_outputs_[vi])),
            size
        };

        for (size_type di = 0; di < size; di++) {
            // Set current variable value for each divide
            var_cache_.at({{time_current_index_, di, vi}}) = var_span[di];
        }
    }

    time_current_index_++;
};

ForcingsEngine& ForcingsEngine::instance(
    const std::string& init,
    std::size_t time_start,
    std::size_t time_end
)
{
    if (inst_ == nullptr) {
        inst_ = std::make_unique<ForcingsEngine>(init, time_start, time_end);
    }

    return *inst_;
}

void ForcingsEngine::check_runtime_requirements()
{
    
    // Check that the python module is installed.
    {
        auto interpreter_ = utils::ngenPy::InterpreterUtil::getInstance();
        try {
            auto mod = interpreter_->getModule("NextGen_Forcings_Engine");
            auto cls = mod.attr("BMIForcingsEngine").cast<py::object>();
        } catch(std::exception& e) {
            throw std::runtime_error{
                "Failed to initialize ForcingsEngine: ForcingsEngine python module is not installed or is not properly configured. (" + std::string{e.what()} + ")"
            };
        }
    }

    // Check that the WGRIB2 environment variable is defined
    {
        const auto* wgrib2_exec = std::getenv("WGRIB2");
        if (wgrib2_exec == nullptr) {
            throw std::runtime_error{"Failed to initialize ForcingsEngine: $WGRIB2 is not defined"};
        }
    }
}

void ForcingsEngine::set_communicator(int handle)
{
    bmi_->set_value<int>("bmi_mpi_comm_handle", { handle });
}

auto ForcingsEngine::outputs() const noexcept
    -> boost::span<const std::string>
{
    return var_outputs_;
}

auto ForcingsEngine::time_begin() const noexcept
    -> const clock_type::time_point&
{
    return time_start_;
}

auto ForcingsEngine::time_end() const noexcept
    -> const clock_type::time_point&
{
    return time_end_;
}

auto ForcingsEngine::time_step() const noexcept
    -> const clock_type::duration&
{
    return time_step_;
}

auto ForcingsEngine::time_index(time_t ctime) const noexcept
    -> size_type
{
    const auto epoch = clock_type::from_time_t(ctime);

    if (epoch < time_begin() || epoch > time_end()) {
        return bad_index;
    }

    const auto offset = (epoch - time_begin()) / time_step();
    return offset;
}

auto ForcingsEngine::divide_index(const std::string& divide_id) const noexcept
    -> size_type
{

    const auto  id_sep   = divide_id.find('-');
    const char* id_split = id_sep == std::string::npos
                           ? divide_id.data()
                           : &divide_id[id_sep + 1];
    const int   id_int   = std::atoi(id_split);

    const auto pos = std::lower_bound(var_divides_.begin(), var_divides_.end(), id_int);
    if (pos == var_divides_.end() || *pos != id_int) {
        return bad_index;
    }

    // implicit cast to unsigned
    return std::distance(var_divides_.begin(), pos);
}

auto ForcingsEngine::variable_index(const std::string& variable) const noexcept
    -> size_type
{
    const auto vars = outputs();
    const auto* pos = std::find_if(vars.begin(), vars.end(), [&](const std::string& var) -> bool {
        return var == variable || var == (variable + "_ELEMENT");
    });
    
    if (pos == vars.end()) {
        return bad_index;
    }

    // implicit cast to unsigned
    return std::distance(vars.begin(), pos);
}

auto ForcingsEngine::at(
    const time_t& raw_time,
    const std::string& divide_id,
    const std::string& variable
) -> double
{
    const auto t_idx = time_index(raw_time);
    const auto i_idx = divide_index(divide_id);
    const auto v_idx = variable_index(variable);
    
    if (t_idx == bad_index || i_idx == bad_index || v_idx == bad_index) {
        const auto shape = var_cache_.shape();
        const auto shape_str = std::to_string(shape[0]) + ", " +
                               std::to_string(shape[1]) + ", " +
                               std::to_string(shape[2]);

        const auto index_str = (t_idx == bad_index ? "?" : std::to_string(t_idx)) + ", " +
                               (i_idx == bad_index ? "?" : std::to_string(i_idx)) + ", " +
                               (v_idx == bad_index ? "?" : std::to_string(v_idx));

        std::string time_str = std::ctime(&raw_time);
        time_str.pop_back();

        throw std::out_of_range{
            "Failed to get ForcingsEngine value at index {" + index_str + "}" +
            "\n  Shape    : {" + shape_str + "}" +
            "\n  Time     : `" + time_str  + "`" +
            "\n  Divide   : `" + divide_id + "`" +
            "\n  Variable : `" + variable  + "`"
        };
    }

    // If time index is past current index, 
    // update until new index and cache values.
    // Otherwise, return from cache.
    while(time_current_index_ < t_idx) {
        time_current_index_++;

        bmi_->UpdateUntil(
            static_cast<double>(time_current_index_ * time_step().count())
        );

        const auto size = var_divides_.size();
        for (size_type vi = 0; vi < var_outputs_.size(); vi++) {
            // Get current values for each variable
            const auto var_span = boost::span<const double>{
                static_cast<double*>(bmi_->GetValuePtr(var_outputs_[vi])),
                size
            };

            for (size_type di = 0; di < size; di++) {
                // Set current variable value for each divide
                var_cache_.at({{time_current_index_, di, vi}}) = var_span[di];
            }
        }
    }
    
    return var_cache_.at({{t_idx, i_idx, v_idx}});
}

// ============================================================================

constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

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
){};

ForcingsEngineDataProvider::ForcingsEngineDataProvider(
    const std::string& init,
    const std::string& time_start,
    const std::string& time_end,
    const std::string& time_fmt
) : ForcingsEngineDataProvider(
    init,
    parse_time(time_start, time_fmt),
    parse_time(time_end, time_fmt)
){};

ForcingsEngineDataProvider::~ForcingsEngineDataProvider() = default;

void ForcingsEngineDataProvider::set_communicator(int handle) {
    this->engine_->set_communicator(handle);
}

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
    for (auto i = start; i < end; i += engine_->time_step()) {
        const auto current_time = ForcingsEngine::clock_type::to_time_t(i);
        values.push_back(
            engine_->at(current_time, selector.get_id(), selector.get_variable_name())
        );
    }

    return values;
}

} // namespace data_access
