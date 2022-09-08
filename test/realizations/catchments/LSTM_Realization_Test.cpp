#ifdef LSTM_TORCH_LIB_TESTS_ACTIVE

#include "gtest/gtest.h"
#include "realizations/catchment/LSTM_Realization.hpp"
#include "utilities/StreamHandler.hpp"
#include "CsvPerFeatureForcingProvider.hpp"

class LSTMRealizationTest : public ::testing::Test {

protected:

    LSTMRealizationTest() {

    }

    ~LSTMRealizationTest() override {

    }
};

/** Test create LSTM Realization and get_response function. */
TEST_F(LSTMRealizationTest, TestLSTMRealization)
{
    utils::StreamHandler output_stream;

    forcing_params forcing_config("./test/data/forcing/cat-10_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
                                   "CsvPerFeature", "2015-12-01 00:00:00", "2015-12-30 23:00:00");
    auto fp = std::make_shared<CsvPerFeatureForcingProvider>(forcing_config);

    std::string catchment_id = "cat-87";

    lstm::lstm_params params{
      35.2607453,
      -80.84020072,
      15.617167
    };

    lstm::lstm_config config{
      "./test/data/model/lstm/sugar_creek_trained.pt",
      "./test/data/model/lstm/input_scaling.csv",
      "./test/data/model/lstm/initial_states.csv",
      false
    };
 
    realization::LSTM_Realization lstm_realization1(catchment_id, fp, output_stream, params, config);

    double flow;

    flow = lstm_realization1.get_response(1, 3600);

    EXPECT_DOUBLE_EQ(0.17717867679512117, flow);

    ASSERT_TRUE(true);
}

#endif  // LSTM_TORCH_LIB_TESTS_ACTIVE
