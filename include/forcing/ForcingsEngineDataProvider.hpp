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

namespace detail {

//! Storage for Forcings Engine-specific BMI instances.
struct ForcingsEngineStorage {
    using key_type   = std::string;
    using bmi_type   = models::bmi::Bmi_Py_Adapter;
    using value_type = std::shared_ptr<bmi_type>;
    
    value_type get(const key_type& key)
    {
        auto pos = data_.find(key);
        if (pos == data_.end()) {
          return nullptr;
        }
        
        return pos->second;
    }

    void set(const key_type& key, value_type value) { data_[key] = value; }

    void clear() { data_.clear(); }

  private:
    //! Instance map of underlying BMI models.
    std::unordered_map<key_type, value_type> data_;
};

static ForcingsEngineStorage forcings_engine_instances{};

} // namespace detail

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

    std::shared_ptr<models::bmi::Bmi_Py_Adapter> model() noexcept
    {
        return bmi_;
    }

    /* Remaining virtual member functions from DataProvider must be implemented
       by derived classes. */

    data_type get_value(const selection_type& selector, data_access::ReSampleMethod m) override = 0;
    
    std::vector<data_type> get_values(const selection_type& selector, data_access::ReSampleMethod m) override = 0;

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

        bmi_ = detail::forcings_engine_instances.get(init);
        if (bmi_ == nullptr) {
            bmi_ = std::make_shared<models::bmi::Bmi_Py_Adapter>(
                "ForcingsEngine",
                init,
                forcings_engine_python_classpath,
                /*allow_exceed_end=*/true,
                /*has_fixed_time_step=*/true,
                utils::getStdOut()
            );
        
            detail::forcings_engine_instances.set(init, bmi_);
        }

        time_step_ = std::chrono::seconds{static_cast<int64_t>(bmi_->GetTimeStep())};
        time_current_index_ = std::chrono::seconds{static_cast<int64_t>(bmi_->GetCurrentTime())} / time_step_;
        var_output_names_ = bmi_->GetOutputVarNames();
    }

    template<typename Derived>
    static std::unique_ptr<ForcingsEngineDataProvider> make_instance(
        const std::string& init,
        const std::string& time_begin,
        const std::string& time_end,
        const std::string& time_fmt = default_time_format
    )
    {
        auto time_begin_epoch = parse_time(time_begin, time_fmt);
        auto time_end_epoch = parse_time(time_end, time_fmt);
        return std::unique_ptr<Derived>{
            new Derived{init, time_begin_epoch, time_end_epoch}
        };
    }

    void next() {
        bmi_->Update();
        time_current_index_++;
    }

    void next(double time) {
      const auto start = bmi_->GetCurrentTime();
      bmi_->UpdateUntil(time);
      const auto end = bmi_->GetCurrentTime();
      time_current_index_ += static_cast<int64_t>(
        (end - start) / bmi_->GetTimeStep()
      );
    }

    //! Forcings Engine instance
    std::shared_ptr<models::bmi::Bmi_Py_Adapter> bmi_ = nullptr;

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
