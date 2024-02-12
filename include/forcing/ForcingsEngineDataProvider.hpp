#ifndef NGEN_FORCING_FORCINGS_ENGINE_DATA_PROVIDER_HPP
#define NGEN_FORCING_FORCINGS_ENGINE_DATA_PROVIDER_HPP
#include <NGenConfig.h>

static_assert(
    ngen::exec_info::with_python,
    "ForcingEngineDataProvider requires Python support."
);

#include "GenericDataProvider.hpp"
#include "bmi/Bmi_Py_Adapter.hpp"

namespace data_access {

struct ForcingsEngineDataProvider
  : public GenericDataProvider
{
    /**
     * @brief Construct a new Forcings Engine Data Provider instance
     * 
     * @param init Path to initialization config
     * @param time_start start time for module as a unix epoch
     * @param time_end end time for module as a unix epoch
     */
    explicit ForcingsEngineDataProvider(const std::string& init, std::size_t time_start, std::size_t time_end);

    /**
     * @brief Construct a new Forcings Engine Data Provider instance
     * 
     * @param init Path to initialization config
     * @param time_start start time for module as a string timestamp
     * @param time_end end time for module as a string timestamp
     * @param fmt format of timestamps
     */
    explicit ForcingsEngineDataProvider(const std::string& init, const std::string& time_start, const std::string& time_end, const std::string& fmt = "%Y-%m-%d %H:%M:%S");

    ~ForcingsEngineDataProvider() override;

    /**
     * @brief Set the MPI communicator that the Forcings Engine should use.
     * 
     * @param handle int representing a communicator handle.
     */
    void set_communicator(int handle);

    auto get_available_variable_names()
      -> boost::span<const std::string> override;

    auto get_data_start_time()
      -> long override;

    auto get_data_stop_time()
      -> long override;

    auto record_duration()
      -> long override;

    auto get_ts_index_for_time(const time_t& epoch_time)
      -> size_t override;

    auto get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m)
      -> double override;

    auto get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
      -> std::vector<double> override;

  private:
    using clock_type = std::chrono::system_clock;
    models::bmi::Bmi_Py_Adapter instance_;
    clock_type::time_point start_;
    clock_type::time_point end_;
    std::vector<std::string> outputs_;
};

} // namespace data_access

#endif // NGEN_FORCING_FORCINGS_ENGINE_DATA_PROVIDER_HPP
