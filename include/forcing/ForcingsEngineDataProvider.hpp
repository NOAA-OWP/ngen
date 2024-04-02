#ifndef NGEN_FORCING_FE_PER_FEATURE_DATA_PROVDER_HPP
#define NGEN_FORCING_FE_PER_FEATURE_DATA_PROVDER_HPP

#include "GenericDataProvider.hpp"
#include "ForcingsEngine.hpp"

namespace data_access {

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

    boost::span<const std::string> get_available_variable_names() override;

    long get_data_start_time() override;

    long get_data_stop_time() override;

    long record_duration() override;

    size_t get_ts_index_for_time(const time_t& epoch_time) override;

    double get_value(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

    std::vector<double> get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m) override;

  private:
    ForcingsEngine* engine_;
};

} // namespace data_access

#endif // NGEN_FORCING_FE_PER_FEATURE_DATA_PROVDER_HPP
