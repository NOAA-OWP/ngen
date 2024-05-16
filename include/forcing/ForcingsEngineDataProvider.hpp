#pragma once

#include <memory>
#include <unordered_map>
#include <chrono>

#include "DataProvider.hpp"
#include "bmi/Bmi_Py_Adapter.hpp"

namespace data_access {

static constexpr auto forcings_engine_python_module = "NextGen_Forcings_Engine";
static constexpr auto forcings_engine_python_class  = "NWMv3_Forcing_Engine_BMI_model";
static constexpr auto forcings_engine_python_classpath = "NextGen_Forcings_Engine.NWMv3_Forcing_Engine_BMI_model";
static constexpr auto default_time_format = "%Y-%m-%d %H:%M:%S";

//! Parse time string from format.
//! Utility function for ForcingsEngineLumpedDataProvider constructor.
time_t parse_time(const std::string& time, const std::string& fmt);

/**
 * Check that requirements for running the forcings engine
 * are available at runtime. If requirements are not available,
 * then this function throws.
 */
void assert_forcings_engine_requirements();

template<typename DataType, typename SelectionType>
struct ForcingsEngineDataProvider
  : public DataProvider<DataType, SelectionType>
{
    using data_type = DataType;
    using selection_type = SelectionType;
    using clock_type = std::chrono::system_clock;

    ~ForcingsEngineDataProvider() = default;

    boost::span<const std::string> get_available_variable_names() override
    {
        return var_output_names_;
    }

    long get_data_start_time() override
    {
        return clock_type::to_time_t(time_begin_);
    }
  
    long get_data_stop_time() override
    {
        return clock_type::to_time_t(time_end_);
    }

    long record_duration() override
    {
        return std::chrono::duration_cast<std::chrono::seconds>(time_step_).count();
    }

    size_t get_ts_index_for_time(const time_t& epoch_time) override
    {
        const auto epoch = clock_type::from_time_t(epoch_time);

        if (epoch < time_begin_ || epoch > time_end_) {
          throw std::out_of_range{
            "epoch " + std::to_string(epoch.time_since_epoch().count()) +
            " out of range of " + std::to_string(time_begin_.time_since_epoch().count()) + ", "
            + std::to_string(time_end_.time_since_epoch().count())
          };
        }

        return (epoch - time_begin_) / time_step_;
    }

    // Temporary (?) function to clear out instances of this type.
    static void finalize_all() {
        instances_.clear();
    }

    /* Remaining virtual member functions from DataProvider must be implemented
       by derived classes. */

    data_type get_value(const selection_type& selector, data_access::ReSampleMethod m) override = 0;
    
    std::vector<data_type> get_values(const selection_type& selector, data_access::ReSampleMethod m) override = 0;


    /* Friend functions */
    static ForcingsEngineDataProvider* instance(
        const std::string& init,
        const std::string& time_begin,
        const std::string& time_end,
        const std::string& time_fmt = default_time_format
    )
    {
        auto& inst = instances_.at(init);
        if (inst != nullptr) {
            assert(inst->time_begin_.time_since_epoch() == std::chrono::seconds{parse_time(time_begin, time_fmt)});
            assert(inst->time_end_.time_since_epoch() == std::chrono::seconds{parse_time(time_end, time_fmt)});
        }

        return inst.get();
    }

  protected:

    // TODO: It may make more sense to have time_begin_seconds and time_end_seconds coalesced into
    //       a single argument: `clock_type::duration time_duration`, since the forcings engine
    //       manages time via a duration rather than time points. !! Need to double check
    ForcingsEngineDataProvider(
      const std::string& init,
      std::size_t time_begin_seconds,
      std::size_t time_end_seconds
    )
      : time_begin_(std::chrono::seconds{time_begin_seconds})
      , time_end_(std::chrono::seconds{time_end_seconds})
    {

        assert_forcings_engine_requirements();

        bmi_ = std::make_unique<models::bmi::Bmi_Py_Adapter>(
            "ForcingsEngine",
            init,
            forcings_engine_python_classpath,
            /*allow_exceed_end=*/true,
            /*has_fixed_time_step=*/true,
            utils::getStdOut()
        );

        time_step_ = std::chrono::seconds{static_cast<int64_t>(bmi_->GetTimeStep())};
        var_output_names_ = bmi_->GetOutputVarNames();
    }

    static ForcingsEngineDataProvider* set_instance(
        const std::string& init,
        std::unique_ptr<ForcingsEngineDataProvider>&& instance
    )
    {
          instances_[init] = std::move(instance);
          return instances_[init].get();
    };

    //! Instance map
    //! @note this map will exist for each of the 
    //!       3 instance types (lumped, gridded, mesh).
    static std::unordered_map<
        std::string,
        std::unique_ptr<ForcingsEngineDataProvider>
    > instances_;

    // TODO: this, or just push the scope on time members up?
    void increment_time()
    {
        time_current_index_++;
    }

    //! Forcings Engine instance
    std::unique_ptr<models::bmi::Bmi_Py_Adapter> bmi_ = nullptr;

    //! Output variable names
    std::vector<std::string> var_output_names_{};

  private:
    //! Initialization config file path
    std::string init_;

    clock_type::time_point time_begin_{};
    clock_type::time_point time_end_{};
    clock_type::duration   time_step_{};
    std::size_t            time_current_index_{};
};

} // namespace data_access
