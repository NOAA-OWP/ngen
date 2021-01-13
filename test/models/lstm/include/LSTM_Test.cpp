#ifdef LSTM_TORCH_LIB_TESTS_ACTIVE

#include "gtest/gtest.h"
#include "lstm/include/LSTM.h"
#include "lstm/include/lstm_params.h"
#include "lstm/include/lstm_config.h"


class LSTMModelTest : public ::testing::Test {

protected:

    LSTMModelTest() {

    }

    ~LSTMModelTest() override {

    }

    void SetUp() override;

    void TearDown() override;

    void setupLSTMModel();

    std::unique_ptr<lstm::lstm_model> model;
};


void LSTMModelTest::SetUp() {

    setupLSTMModel();    

}

void LSTMModelTest::TearDown() {

}

/** Construct a LSTM Model. */
void LSTMModelTest::setupLSTMModel()
{
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
 
    model = std::make_unique<lstm::lstm_model>(lstm::lstm_model(config, params));

}

/** Test running LSTM model for one timestep. */
TEST_F(LSTMModelTest, TestLSTMModel)
{
    int error = model->run(3600.0, 369.20001220703125, 99870.0, 0.009800000116229057, 
                           9.493307095661946e-08, 0.0, 287.0, -1.7000000476837158, 3.4000000953674316); 

    double outflow = model->get_fluxes()->flow;

    EXPECT_DOUBLE_EQ (0.17238743535773413, outflow);

    ASSERT_TRUE(true);
}

#endif  // LSTM_TORCH_LIB_TESTS_ACTIVE

