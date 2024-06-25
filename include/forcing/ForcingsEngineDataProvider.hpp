#pragma once

#include <memory>
#include <unordered_map>
#include <chrono>

#include "DataProvider.hpp"
#include "bmi/Bmi_Py_Adapter.hpp"

namespace data_access {

//! Python module name for NextGen Forcings Engine
static constexpr auto forcings_engine_python_module = "NextGen_Forcings_Engine";

//! Python classname for NextGen Forcings Engine BMI class
static constexpr auto forcings_engine_python_class  = "NWMv3_Forcing_Engine_BMI_model";

//! Full Python classpath for Forcings Engine BMI class
static constexpr auto forcings_engine_python_classpath = "NextGen_Forcings_Engine.NWMv3_Forcing_Engine_BMI_model";

//! Default time format used for parsing timestamp strings
static constexpr auto default_time_format = "%Y-%m-%d %H:%M:%S";

namespace detail {

//! Parse time string from format.
//! Utility function for ForcingsEngineLumpedDataProvider constructor.
time_t parse_time(const std::string& time, const std::string& fmt);

//! Check that requirements for running the forcings engine
//! are available at runtime. If requirements are not available,
//! then this function throws.
void assert_forcings_engine_requirements();

//! Storage for Forcings Engine-specific BMI instances.
struct ForcingsEngineStorage {
    using key_type   = std::string;
    using bmi_type   = models::bmi::Bmi_Py_Adapter;
    using value_type = std::shared_ptr<bmi_type>;

    static ForcingsEngineStorage instances;

    value_type get(const key_type& key)
    {
        auto pos = data_.find(key);
        if (pos == data_.end()) {
          return nullptr;
        }

        return pos->second;
    }

    void set(const key_type& key, value_type value)
    {
        data_[key] = value;
    }

    void clear()
    {
        data_.clear();
    }

  private:
    //! Instance map of underlying BMI models.
    std::unordered_map<key_type, value_type> data_;
};

} // namespace detail


//! Forcings Engine Data Provider
//! @tparam DataType Data type for values returned from derived classes
//! @tparam SelectionType Selector type for querying data from derived classes
template<typename DataType, typename SelectionType>
struct ForcingsEngineDataProvider
  : public DataProvider<DataType, SelectionType>
{
    using data_type = DataType;
    using selection_type = SelectionType;
    using clock_type = std::chrono::system_clock;

    ~ForcingsEngineDataProvider() override = default;

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

    data_type get_value(
      const selection_type& selector,
      data_access::ReSampleMethod m
    ) override = 0;

    std::vector<data_type> get_values(
      const selection_type& selector,
      data_access::ReSampleMethod m
    ) override = 0;

  protected:
    using storage_type = detail::ForcingsEngineStorage;

    ForcingsEngineDataProvider(
        std::string init,
        std::size_t time_begin_seconds,
        std::size_t time_end_seconds
    )
      : time_begin_(std::chrono::seconds{time_begin_seconds})
      , time_end_(std::chrono::seconds{time_end_seconds})
    {
        // Get a forcings engine instance if it exists for this initialization file
        auto bmi_ = storage_type::instances.get(init);

        // If it doesn't exist, create it and assign it to the storage map
        if (bmi_ == nullptr) {
            // Outside of this branch, this->bmi_ != nullptr after this
            bmi_ = std::make_shared<models::bmi::Bmi_Py_Adapter>(
                "ForcingsEngine",
                init,
                forcings_engine_python_classpath,
                /*allow_exceed_end=*/true,
                /*has_fixed_time_step=*/true,
                utils::getStdOut()
            );

            storage_type::instances.set(init, bmi_);
        }

        // Now, initialize the BMI dependent instance members
        // NOTE: using std::lround instead of static_cast will prevent potential UB
        time_step_ = std::chrono::seconds{std::lround(bmi_->GetTimeStep())};
        time_current_index_ = (std::chrono::seconds{std::lround(bmi_->GetCurrentTime())} / time_step_); // resolves to long
        var_output_names_ = bmi_->GetOutputVarNames();
    }

    
    //! Update the Forcings Engine instance to the next timestep.
    void next()
    {
        bmi_->Update();
        time_current_index_++;
    }

    //! Update the Forcings Engine instance to the next timestep before `time`.
    //! @param time Time in seconds to update to.
    //!             i.e. A value of 14401 will update
    //!                  the instance to the 5th time index,
    //!                  since 3600 * 4 = 14400 but 14400 < 14401.
    void next(double time)
    {
        const auto start = bmi_->GetCurrentTime();
        bmi_->UpdateUntil(time);
        const auto end = bmi_->GetCurrentTime();
        const auto diff = end - start;
        time_current_index_ += std::chrono::seconds{std::lround(diff)} / time_step_;
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
