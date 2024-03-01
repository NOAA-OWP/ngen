#ifndef NGEN_FORCING_FE_PER_FEATURE_DATA_PROVDER_HPP
#define NGEN_FORCING_FE_PER_FEATURE_DATA_PROVDER_HPP

#include "GenericDataProvider.hpp"

namespace data_access {

struct ForcingsEngine;

struct ForcingsEngineDataProvider
  : public GenericDataProvider
{

    ForcingsEngineDataProvider() = default;

    ForcingsEngineDataProvider(
        const std::string& init,
        std::size_t time_start,
        std::size_t time_end
    );

    ForcingsEngineDataProvider(
        const std::string& init,
        const std::string& time_start,
        const std::string& time_end,
        const std::string& time_fmt = "%Y-%m-%d %H:%M:%S"
    );

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
    ForcingsEngine* engine_;
};

} // namespace data_access

#endif // NGEN_FORCING_FE_PER_FEATURE_DATA_PROVDER_HPP
