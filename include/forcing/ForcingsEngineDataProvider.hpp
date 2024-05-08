#pragma once

#include <memory>
#include <unordered_map>
#include <chrono>

#include "DataProvider.hpp"
#include "bmi/Bmi_Py_Adapter.hpp"

namespace data_access {

/**
 * Check that requirements for running the forcings engine
 * are available at runtime. If requirements are not available,
 * then this function throws.
 */
void assert_forcings_engine_requirements();

template<typename data_type, typename selection_type>
struct ForcingsEngineDataProvider
  : public DataProvider<data_type, selection_type>
{
    using clock_type = std::chrono::system_clock;

    ForcingsEngineDataProvider(
      const std::string& init,
      std::size_t time_begin_seconds,
      std::size_t time_end_seconds
    )
      : time_begin_(std::chrono::seconds{time_begin_seconds})
      , time_end_(std::chrono::seconds{time_end_seconds})
    {
        bmi_ = std::make_unique<models::bmi::Bmi_Py_Adapter>(
            "ForcingsEngine",
            init,
            "NextGen_Forcings_Engine.BMIForcingsEngine",
            /*allow_exceed_end=*/true,
            /*has_fixed_time_step=*/true,
            utils::getStdOut()
        );

        time_step_ = std::chrono::seconds{static_cast<int64_t>(bmi_->GetTimeStep())};
        var_output_names_ = bmi_->GetOutputVarNames();
    }

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
        return time_step_.count();
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

    double get_value(const selection_type& selector, data_access::ReSampleMethod m) override = 0;
    
    std::vector<double> get_values(const selection_type& selector, data_access::ReSampleMethod m) override = 0;

  protected:
    //! Instance map
    //! @note this map will exist for each of the 
    //!       3 instance types (lumped, gridded, mesh).
    static std::unordered_map<
        std::string,
        std::unique_ptr<ForcingsEngineDataProvider>
    > instances_;

    static ForcingsEngineDataProvider* get_instance(
      const std::string& init,
      std::size_t time_begin,
      std::size_t time_end
    )
    {
        std::unique_ptr<ForcingsEngineDataProvider>& inst = instances_.at(init);

        if (inst != nullptr) {
          assert(inst->time_begin_ == clock_type::time_point{std::chrono::seconds{time_begin}});
          assert(inst->time_end_ == clock_type::time_point{std::chrono::seconds{time_begin}});
        }

        return inst.get();
    }

    template<typename Callable>
    static ForcingsEngineDataProvider* set_instance(const std::string& init, Callable&& f)
    {
        instances_[init] = f(init);
        return instances_[init].get();
    }

    // TODO: this, or just push the scope on time members up?
    void increment_time()
    {
        time_current_index_++;
    }

    //! Forcings Engine instance
    std::unique_ptr<models::bmi::Bmi_Py_Adapter> bmi_ = nullptr;

  private:
    //! Initialization config file path
    std::string init_;

    clock_type::time_point time_begin_{};
    clock_type::time_point time_end_{};
    clock_type::duration   time_step_{};
    std::size_t            time_current_index_{};

    //! Output variable names
    std::vector<std::string> var_output_names_{};
};

} // namespace data_access
