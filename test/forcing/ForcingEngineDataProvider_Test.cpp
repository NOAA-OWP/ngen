#include <gtest/gtest.h>

#include "AorcForcing.hpp"
#include "DataProvider.hpp"
#include "ForcingEngineDataProvider.hpp"

#include <boost/range/combine.hpp>

TEST(ForcingEngineDataProviderTest, Initialization) {
    auto interp_ = utils::ngenPy::InterpreterUtil::getInstance();

    data_access::ForcingEngineDataProvider provider{
        "extern/ngen-forcing/NextGen_Forcings_Engine_BMI/NextGen_Forcings_Engine/config.yml",
        "2024-01-17 01:00:00",
        "2024-01-17 06:00:00"  
    };

    const auto outputs_test = provider.get_available_variable_names();
    const auto outputs_expected = {
        "CAT-ID",
        "U2D_ELEMENT",
        "V2D_ELEMENT",
        "LWDOWN_ELEMENT",
        "SWDOWN_ELEMENT",
        "T2D_ELEMENT",
        "Q2D_ELEMENT",
        "PSFC_ELEMENT",
        "RAINRATE_ELEMENT"
    };
    
    for (decltype(auto) output : boost::combine(outputs_test, outputs_expected)) {
        EXPECT_EQ(output.get<0>(), output.get<1>());
    }

    forcing_params params{"", "ForcingsEngine", "2024-01-17 01:00:00", "2024-01-17 06:00:00"};

    EXPECT_EQ(provider.get_ts_index_for_time(params.simulation_start_t), 0);
    EXPECT_EQ(provider.get_ts_index_for_time(params.simulation_end_t), 5);

    const auto selector = CatchmentAggrDataSelector{
        "cat-1015786",
        "RAINRATE",
        provider.get_data_start_time(),
        static_cast<long>(provider.get_ts_index_for_time(params.simulation_end_t) * 3600),
        "seconds"
    };

    const auto result = provider.get_values(
        selector,
        data_access::ReSampleMethod::SUM
    );

    for (const auto& r : result) {
        EXPECT_NEAR(r, 5.51193e-07, 1e6);
    }

    const auto lookback = provider.get_value(selector, data_access::ReSampleMethod::SUM);
    std::cout << "Lookback: " << lookback << '\n';
    EXPECT_EQ(lookback, std::accumulate(result.begin(), result.end(), 0.0));
}
