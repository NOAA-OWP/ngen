#include <gtest/gtest.h>

#include "ForcingEngineDataProvider.hpp"

#include <boost/range/combine.hpp>

TEST(ForcingEngineDataProviderTest, Initialization) {
    auto interp_ = utils::ngenPy::InterpreterUtil::getInstance();

    data_access::ForcingEngineDataProvider provider{
        "extern/ngen-forcing/NextGen_Forcings_Engine_BMI/NextGen_Forcings_Engine/config.yml"
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
}
