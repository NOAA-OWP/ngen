#include "ForcingsEngine.hpp"

namespace data_access {

std::unordered_map<std::string, std::unique_ptr<ForcingsEngine>> ForcingsEngine::instances_{};

ForcingsEngine::ForcingsEngine(const std::string& init, size_type time_start, size_type time_end)
  : time_start_(std::chrono::seconds{time_start})
  , time_end_(std::chrono::seconds{time_end})
{
    bmi_ = std::make_unique<models::bmi::Bmi_Py_Adapter>(
        "ForcingsEngine",
        init,
        "NextGen_Forcings_Engine.BMIForcingsEngine",
        true, /* allow_exceed_end */
        true, /* has_fixed_time_step */
        utils::getStdOut()
    );
    
    time_step_ = std::chrono::seconds{static_cast<int64_t>(bmi_->GetTimeStep())};
    var_outputs_ = bmi_->GetOutputVarNames();

    auto cat_var_pos = std::find(var_outputs_.begin(), var_outputs_.end(), "CAT-ID");
    if (cat_var_pos == var_outputs_.end()) {
        throw std::runtime_error{
            "Failed to initialize ForcingsEngine: `CAT-ID` is not an output variable of the forcings engine."
            " Is it running with `GRID_TYPE` set to 'hydrofabric'?"
        };
    }
    var_outputs_.erase(cat_var_pos); // Erase "CAT-ID" from outputs


    const auto time_dim = static_cast<size_type>((time_end_ - time_start_) / time_step_);
    const auto id_dim   = static_cast<size_type>(bmi_->GetVarNbytes("CAT-ID") / bmi_->GetVarItemsize("CAT-ID"));
    const auto var_dim  = var_outputs_.size();

    bmi_->Update();
    time_current_index_++;
    
    const auto* ptr = static_cast<int*>(bmi_->GetValuePtr("CAT-ID"));
    var_divides_ = std::vector<int>(ptr, ptr + id_dim);
    var_cache_ = decltype(var_cache_){{ 2, id_dim, var_dim }};

    // Cache initial iteration
    update_value_storage_();
    time_current_index_++;
}

ForcingsEngine::~ForcingsEngine()
{
    finalize();
}

ForcingsEngine& ForcingsEngine::instance(
    const std::string& init,
    std::size_t time_start,
    std::size_t time_end
)
{
    std::unique_ptr<ForcingsEngine>& inst = instances_[init];
    if (inst == nullptr) {
        inst = std::make_unique<ForcingsEngine>(init, time_start, time_end);
    }

    assert(inst->time_start_ == ForcingsEngine::clock_type::time_point{std::chrono::seconds{time_start}});
    assert(inst->time_end_ == ForcingsEngine::clock_type::time_point{std::chrono::seconds{time_end}});
    return *inst;
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

void ForcingsEngine::update_value_storage_()
{
    const auto size = var_divides_.size();
    for (size_type vi = 0; vi < var_outputs_.size(); vi++) {
        // Get current values for each variable
        const auto var_span = boost::span<const double>{
            static_cast<double*>(bmi_->GetValuePtr(var_outputs_[vi])),
            size
        };

        for (size_type di = 0; di < size; di++) {
            // Move 0 time (current) to 1 (previous), and set the
            // current values from var_span
            var_cache_.at({{1, di, vi}}) = var_cache_.at({{0, di, vi}});
            var_cache_.at({{0, di, vi}}) = var_span[di];
        }
    }
}

boost::span<const std::string> ForcingsEngine::outputs() const noexcept
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
    return time_index(epoch);
}

auto ForcingsEngine::time_index(clock_type::time_point epoch) const noexcept
  -> size_type
{
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
                           ? &divide_id[0]
                           : &divide_id[id_sep + 1];
    const int   id_int   = std::atoi(id_split);

    const auto pos = std::find(var_divides_.begin(), var_divides_.end(), id_int);
    if (pos == var_divides_.end()) {
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
        // Checks for a variable name, plus the case where only the prefix is given, i.e.
        // both variable_index("U2D") and variable_index("U2D_ELEMENT") will work.
        return var == variable || var == (variable + "_ELEMENT");
    });
    
    if (pos == vars.end()) {
        return bad_index;
    }

    // implicit cast to unsigned
    return std::distance(vars.begin(), pos);
}

bool ForcingsEngine::next()
{
    bmi_->Update();
    time_current_index_++;
    update_value_storage_();
    return true;
}

double ForcingsEngine::at(
    const std::string& divide_id,
    const std::string& variable,
    bool               previous
)
{
    const auto d_idx = divide_index(divide_id);
    const auto v_idx = variable_index(variable);

    if (d_idx == bad_index || v_idx == bad_index) {
        const auto shape = var_cache_.shape();
        const auto shape_str = std::to_string(shape[0]) + ", " +
                               std::to_string(shape[1]) + ", " +
                               std::to_string(shape[2]);

        const auto index_str = (d_idx == bad_index ? "?" : std::to_string(d_idx)) + ", " +
                               (v_idx == bad_index ? "?" : std::to_string(v_idx));

        throw std::out_of_range{
            "Failed to get ForcingsEngine value at index {" + index_str + "}" +
            "\n  Shape    : {" + shape_str + "}" +
            "\n  Divide   : `" + divide_id + "`" +
            "\n  Variable : `" + variable  + "`"
        };
    }

    return at(d_idx, v_idx, previous);
}

double ForcingsEngine::at(
    size_type divide_idx,
    size_type variable_idx,
    bool      previous
)
{
    if (divide_idx == bad_index || variable_idx == bad_index) {
        const auto shape = var_cache_.shape();
        const auto shape_str = std::to_string(shape[0]) + ", " +
                               std::to_string(shape[1]) + ", " +
                               std::to_string(shape[2]);

        const auto index_str = (divide_idx == bad_index ? "?" : std::to_string(divide_idx)) + ", " +
                               (variable_idx == bad_index ? "?" : std::to_string(variable_idx));

        throw std::out_of_range{
            "Failed to get ForcingsEngine value at index {" + index_str + "}" +
            "\n  Shape          : {" + shape_str + "}"
        };
    }
    
    return var_cache_.at({{previous ? 1U : 0U, divide_idx, variable_idx}});
}

void ForcingsEngine::finalize()
{
    bmi_->Finalize();
}


void ForcingsEngine::finalize_all()
{
    instances_.clear();
}

} // namespace data_access
