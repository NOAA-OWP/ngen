/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
------------------------------------------------------------------------
*/
#ifdef NGEN_BMI_CPP_LIB_TESTS_ACTIVE
// Test utitilities
//  #include "bmi.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
// Mock configuration and context helpers
#include "MockConfig.hpp"
// BMI model/adapter
#include "Bmi_Cpp_Adapter.hpp"
#include "FileChecker.h"
#include <vector>
// Interface under test
#include "protocols.hpp"

using ::testing::MatchesRegex;
// protocol symbols
using models::bmi::protocols::INPUT_MASS_NAME;
using models::bmi::protocols::LEAKED_MASS_NAME;
using models::bmi::protocols::NgenBmiProtocols;
using models::bmi::protocols::OUTPUT_MASS_NAME;
using models::bmi::protocols::ProtocolError;
using models::bmi::protocols::STORED_MASS_NAME;
using nonstd::expected_lite::expected;

// Use the ngen bmi c++ test library
#ifndef BMI_TEST_CPP_LOCAL_LIB_NAME
#ifdef __APPLE__
#define BMI_TEST_CPP_LOCAL_LIB_NAME "libtestbmicppmodel.dylib"
#else
#ifdef __GNUC__
#define BMI_TEST_CPP_LOCAL_LIB_NAME "libtestbmicppmodel.so"
#endif // __GNUC__
#endif // __APPLE__
#endif // BMI_TEST_CPP_LOCAL_LIB_NAME

#define CREATOR_FUNC   "bmi_model_create"
#define DESTROYER_FUNC "bmi_model_destroy"

using namespace models::bmi;

// Copy of the struct def used within the test_bmi_c test library

class Bmi_Cpp_Test_Adapter : public ::testing::Test {
  protected:
    void SetUp() override;

    void TearDown() override;

    std::string file_search(
        const std::vector<std::string>& parent_dir_options,
        const std::string& file_basename
    );

    std::string config_file_name_0;
    std::string lib_file_name_0;
    std::string bmi_module_type_name_0;
    std::unique_ptr<Bmi_Cpp_Adapter> adapter;
};

void Bmi_Cpp_Test_Adapter::SetUp() {
    /**
     * @brief Set up the test environment for the protocol tests.
     *
     * The protocol requires a valid BMI model to operate on, so this setup
     * function initializes a BMI C++ adapter using the test BMI C++ model
     *
     */
    // Uses the same config files as the C test model...
    std::vector<std::string> config_path_options = {
        "test/data/bmi/test_bmi_c/",
        "./test/data/bmi/test_bmi_c/",
        "../test/data/bmi/test_bmi_c/",
        "../../test/data/bmi/test_bmi_c/",
    };
    std::string config_basename_0 = "test_bmi_c_config_0.txt";
    config_file_name_0            = file_search(config_path_options, config_basename_0);

    std::vector<std::string> lib_dir_opts = {
        "./extern/test_bmi_cpp/cmake_build/",
        "../extern/test_bmi_cpp/cmake_build/",
        "../../extern/test_bmi_cpp/cmake_build/"
    };
    lib_file_name_0        = file_search(lib_dir_opts, BMI_TEST_CPP_LOCAL_LIB_NAME);
    bmi_module_type_name_0 = "test_bmi_cpp";
    try {
        adapter = std::make_unique<Bmi_Cpp_Adapter>(
            bmi_module_type_name_0,
            lib_file_name_0,
            config_file_name_0,
            true,
            CREATOR_FUNC,
            DESTROYER_FUNC
        );
    } catch (const std::exception& e) {
        std::clog << e.what() << std::endl;
        throw e;
    }
}

void Bmi_Cpp_Test_Adapter::TearDown() {
}

std::string Bmi_Cpp_Test_Adapter::file_search(
    const std::vector<std::string>& parent_dir_options,
    const std::string& file_basename
) {
    // Build vector of names by building combinations of the path and basename options
    std::vector<std::string> name_combinations;

    // Build so that all path names are tried for given basename before trying a different basename
    // option
    for (auto& path_option : parent_dir_options)
        name_combinations.push_back(path_option + file_basename);

    return utils::FileChecker::find_first_readable(name_combinations);
}

class Bmi_Mass_Balance_Test : public Bmi_Cpp_Test_Adapter {
  protected:
    void SetUp() override {
        Bmi_Cpp_Test_Adapter::SetUp();
        model      = std::shared_ptr<models::bmi::Bmi_Adapter>(adapter.release());
        model_name = model->GetComponentName();
    }

    std::string time = "t0";
    std::string model_name;
    std::shared_ptr<models::bmi::Bmi_Adapter> model;
};

TEST_F(Bmi_Mass_Balance_Test, bad_model) {
    model           = nullptr; // simulate uninitialized model
    auto properties = MassBalanceMock(true).as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    testing::internal::CaptureStderr();
    auto protocols     = NgenBmiProtocols(model, properties);
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_THAT(
        output,
        MatchesRegex("Error\\(Uninitialized Model\\).*Disabling mass balance protocol.\n")
    );
    testing::internal::CaptureStderr();
    auto result = protocols.run(models::bmi::protocols::Protocol::MASS_BALANCE, context);
    EXPECT_TRUE(!result.has_value()); // should have an error!!!
    EXPECT_EQ(result.error().error_code(), models::bmi::protocols::Error::UNITIALIZED_MODEL);
    output = testing::internal::GetCapturedStderr();
    EXPECT_THAT(output, MatchesRegex("Error\\(Uninitialized Model\\).*\n"));
}

TEST_F(Bmi_Mass_Balance_Test, default_construct) {
    auto context   = make_context(0, 2, time, model_name);
    auto protocols = NgenBmiProtocols();
    testing::internal::CaptureStderr();
    auto result = protocols.run(models::bmi::protocols::Protocol::MASS_BALANCE, context);
    EXPECT_TRUE(!result.has_value()); // should have an error!!!
    EXPECT_EQ(result.error().error_code(), models::bmi::protocols::Error::UNITIALIZED_MODEL);
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_THAT(output, MatchesRegex("Error\\(Uninitialized Model\\).*\n"));
}

TEST_F(Bmi_Mass_Balance_Test, check) {
    auto properties = MassBalanceMock(true).as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);
    (void)protocols.run(models::bmi::protocols::Protocol::MASS_BALANCE, context);
    model->Update();
    time = "t1";
    (void)protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(1, 2, time, model_name)
    );
    std::string output = testing::internal::GetCapturedStderr();
    // Not warning/errors printed, and no exceptions thrown is success
    EXPECT_EQ(output, "");
}

TEST_F(Bmi_Mass_Balance_Test, warns) {
    auto properties = MassBalanceMock(false).as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    testing::internal::CaptureStderr();
    auto protocols    = NgenBmiProtocols(model, properties);
    double mass_error = 100;
    model->SetValue(STORED_MASS_NAME, &mass_error); // Force a mass balance error
    auto result = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(1, 2, time, model_name)
    );
    // The expected should be an error not a value (void in this case)
    EXPECT_FALSE(result.has_value()); // should have a warning!!!
    EXPECT_THAT(result.error().to_string(), testing::HasSubstr("Warning(Protocol)::mass_balance:"));
    std::string output = testing::internal::GetCapturedStderr();
    // std::cerr << output;
    // Warning was sent to stderr
    EXPECT_THAT(output, testing::HasSubstr("Warning(Protocol)::mass_balance:"));
}

TEST_F(Bmi_Mass_Balance_Test, storage_fails) {
    auto properties = MassBalanceMock(true, 1e-5).as_json_property();
    auto context    = make_context(0, 2, time, model_name);

    auto protocols = NgenBmiProtocols(model, properties);
    model->Update(); // advance model
    double mass_error = 2;
    model->SetValue(STORED_MASS_NAME, &mass_error); // Force a mass balance error
    time = "t1";

    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        ),
        ProtocolError
    );
    try {
        // This should throw, so result won't be defined...
        auto result = protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        );
    } catch (ProtocolError& e) {
        // std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(
            e.to_string(),
            MatchesRegex("Error\\(Protocol\\)::mass_balance: at timestep 1 \\(t1\\).*")
        );
    }
}

TEST_F(Bmi_Mass_Balance_Test, in_fails) {
    auto properties = MassBalanceMock(true, 1e-5).as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    auto protocols  = NgenBmiProtocols(model, properties);
    model->Update(); // advance model
    time              = "t1";
    double mass_error = 2;
    model->SetValue(INPUT_MASS_NAME, &mass_error); // Force a mass balance error
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        ),
        ProtocolError
    );
    try {
        // This should throw, so result won't be defined...
        auto result = protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        );
    } catch (ProtocolError& e) {
        // std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(
            e.to_string(),
            MatchesRegex("Error\\(Protocol\\)::mass_balance: at timestep 1 \\(t1\\).*")
        );
    }
}

TEST_F(Bmi_Mass_Balance_Test, out_fails) {
    auto properties = MassBalanceMock(true, 1e-5).as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    auto protocols  = NgenBmiProtocols(model, properties);
    model->Update(); // advance model
    time              = "t1";
    double mass_error = 2;
    model->SetValue(OUTPUT_MASS_NAME, &mass_error); // Force a mass balance error
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        ),
        ProtocolError
    );
    try {
        // This should throw, so result won't be defined...
        auto result = protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        );
    } catch (ProtocolError& e) {
        // std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(
            e.to_string(),
            MatchesRegex("Error\\(Protocol\\)::mass_balance: at timestep 1 \\(t1\\).*")
        );
    }
}

TEST_F(Bmi_Mass_Balance_Test, leaked_fails) {
    auto properties = MassBalanceMock(true, 1e-5).as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    auto protocols  = NgenBmiProtocols(model, properties);
    model->Update(); // advance model
    time              = "t1";
    double mass_error = 2;
    model->SetValue(LEAKED_MASS_NAME, &mass_error); // Force a mass balance error
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        ),
        ProtocolError
    );
    try {
        // This should throw, so result won't be defined...
        auto result = protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        );
    } catch (ProtocolError& e) {
        // std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(
            e.to_string(),
            MatchesRegex("Error\\(Protocol\\)::mass_balance: at timestep 1 \\(t1\\).*")
        );
    }
}

TEST_F(Bmi_Mass_Balance_Test, tolerance_fails) {
    auto properties = MassBalanceMock(true, 1e-5).as_json_property();
    ;
    auto context   = make_context(0, 2, time, model_name);
    auto protocols = NgenBmiProtocols(model, properties);
    model->Update(); // advance model
    double mass_error;
    model->GetValue(INPUT_MASS_NAME, &mass_error);
    mass_error += 1e-4; // Force a mass balance error not within tolerance
    model->SetValue(INPUT_MASS_NAME, &mass_error); // Force a mass balance error
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(0, 2, time, model_name)
        ),
        ProtocolError
    );
    try {
        // This should throw, so result won't be defined...
        auto result = protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(0, 2, time, model_name)
        );
    } catch (ProtocolError& e) {
        // std::cerr << e.to_string() << std::endl;
        EXPECT_THAT(
            e.to_string(),
            MatchesRegex("Error\\(Protocol\\)::mass_balance: at timestep 0 \\(t0\\).*")
        );
    }
}

TEST_F(Bmi_Mass_Balance_Test, tolerance_passes) {
    auto properties = MassBalanceMock(true, 1e-5).as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    auto protocols  = NgenBmiProtocols(model, properties);
    model->Update(); // advance model
    double mass_error;
    model->GetValue(INPUT_MASS_NAME, &mass_error);
    mass_error += 1e-6; // Force a mass balance error within tolerance
    model->SetValue(INPUT_MASS_NAME, &mass_error); // Force a mass balance error
    // should not thow!
    auto result = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(0, 2, time, model_name)
    );
    EXPECT_TRUE(result.has_value()); // should pass
}

TEST_F(Bmi_Mass_Balance_Test, disabled) {
    auto properties = MassBalanceMock(true, 1e-5, 1, false).as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);

    auto result = protocols.run(models::bmi::protocols::Protocol::MASS_BALANCE, context);
    EXPECT_TRUE(result.has_value()); // should pass, as mass balance is disabled
    model->Update(); // advance model
    time              = "t1";
    double mass_error = 100;
    model->SetValue(STORED_MASS_NAME, &mass_error); // Force a mass balance error
    result = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(1, 2, time, model_name)
    );
    // should not throw, as mass balance is disabled
    EXPECT_TRUE(result.has_value()); // should pass, as mass balance is disabled
    std::string output = testing::internal::GetCapturedStderr();
    // No warnings/errors printed, and no exceptions thrown is success
    EXPECT_EQ(output, "");
}

TEST_F(Bmi_Mass_Balance_Test, unconfigured) {
    auto properties = noneConfig;
    auto context    = make_context(0, 2, time, model_name);
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);

    auto result = protocols.run(models::bmi::protocols::Protocol::MASS_BALANCE, context);
    EXPECT_TRUE(result.has_value()); // should pass, as mass balance is disabled
    model->Update(); // advance model
    time              = "t1";
    double mass_error = 100;
    model->SetValue(STORED_MASS_NAME, &mass_error); // Force a mass balance error
    result = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(1, 2, time, model_name)
    );
    // should not throw, as mass balance is disabled
    EXPECT_TRUE(result.has_value()); // should pass, as mass balance is disabled
    std::string output = testing::internal::GetCapturedStderr();
    // No warnings/errors printed, and no exceptions thrown is success
    EXPECT_EQ(output, "");
}

TEST_F(Bmi_Mass_Balance_Test, frequency) {
    auto properties   = MassBalanceMock(true, 1e-5, 2).as_json_property();
    auto context      = make_context(0, 2, time, model_name);
    auto protocols    = NgenBmiProtocols(model, properties);
    double mass_error = 10; // Force a mass balance error above tolerance
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    // Check initial mass balance -- should error which indicates it was propoerly checked
    // per frequency setting
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(0, 2, time, model_name)
        ),
        ProtocolError
    );
    time = "t1";
    model->Update(); // advance model
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    // Call mass balance check again, this should NOT error, since the actual check
    // should be skipped due to the frequency setting
    auto result = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(1, 2, time, model_name)
    );
    EXPECT_TRUE(result.has_value()); // should pass
    time = "t2";
    model->Update(); // advance model
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    // Check mass balance again, this SHOULD error since the previous mass balance
    // will propagate, and it should now be checked based on the frequency
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(2, 2, time, model_name)
        ),
        ProtocolError
    );
}

TEST_F(Bmi_Mass_Balance_Test, frequency_zero) {
    auto properties   = MassBalanceMock(true, 1e-5, 0).as_json_property();
    auto context      = make_context(0, 2, time, model_name);
    auto protocols    = NgenBmiProtocols(model, properties);
    double mass_error = 10; // Force a mass balance error above tolerance
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    auto result = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(0, 2, time, model_name)
    );
    EXPECT_TRUE(result.has_value()); // should pass, frequency 0 means never check
}

TEST_F(Bmi_Mass_Balance_Test, frequency_end) {
    // Context contract: current_time_step is drawn from [0, total_steps - 1].
    // For a 3-step run, indices 0 and 1 must skip the check (frequency=-1
    // only fires at the end) and index 2 must fire, since
    // total_steps - 1 == 2 is the last in-range index.
    auto properties   = MassBalanceMock(true, 1e-5, -1).as_json_property();
    auto protocols    = NgenBmiProtocols(model, properties);
    double mass_error = 10; // Force a mass balance error above tolerance
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    // Check initial mass balance -- should not error due to frequency setting
    auto result = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(0, 3, time, model_name)
    );
    EXPECT_TRUE(result.has_value()); // should pass
    time = "t1";
    model->Update(); // advance model
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    // Call mass balance check again, this should NOT error, since the actual check
    // should be skipped due to the frequency setting
    result = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(1, 3, time, model_name)
    );
    EXPECT_TRUE(result.has_value()); // should pass
    time = "t2";
    model->Update(); // advance model
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    // Check mass balance again, this SHOULD error since its the last timestep
    // (current_time_step == total_steps - 1) and the mass balance violation
    // should now be observable.
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(2, 3, time, model_name)
        ),
        ProtocolError
    );
}

TEST_F(Bmi_Mass_Balance_Test, frequency_end_single_step) {
    // Boundary case: N=1, where the first step is also the last.
    // current_time_step=0 and total_steps=1, so the sentinel
    // (current_time_step == total_steps - 1) is `0 == 0` and the check
    // must fire. The pre-fix comparison `current == total` was `0 == 1`
    // here — false — so a single-step run with frequency=-1 would have
    // silently skipped the check, even though it's the only chance to
    // verify mass balance.
    auto properties   = MassBalanceMock(true, 1e-5, -1).as_json_property();
    auto protocols    = NgenBmiProtocols(model, properties);
    double mass_error = 10; // Force a mass balance error above tolerance
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(0, 1, time, model_name)
        ),
        ProtocolError
    );
}

TEST_F(Bmi_Mass_Balance_Test, frequency_end_large_n) {
    // Confirm that across a longer run the end-sentinel fires exactly once,
    // on the last step, and not on any intermediate step. Steps [0, N-2)
    // must not throw (check skipped); step N-1 must throw (violation
    // observed).
    auto properties   = MassBalanceMock(true, 1e-5, -1).as_json_property();
    auto protocols    = NgenBmiProtocols(model, properties);
    double mass_error = 10; // Force a mass balance error above tolerance

    constexpr int N = 5;
    for (int k = 0; k < N - 1; ++k) {
        model->SetValue(OUTPUT_MASS_NAME, &mass_error);
        auto r = protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(k, N, "t" + std::to_string(k), model_name)
        );
        EXPECT_TRUE(r.has_value()) << "frequency=-1 must not fire at step " << k;
        model->Update();
    }
    // Last step: violation must be observed.
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(N - 1, N, "t" + std::to_string(N - 1), model_name)
        ),
        ProtocolError
    );
}

TEST_F(Bmi_Mass_Balance_Test, frequency_negative_other_than_minus_one) {
    // Any negative frequency takes the same end-of-run branch — the protocol
    // only special-cases the `frequency > 0` modulo path; the `else` branch
    // is reached for all non-positive frequencies (and `frequency == 0` is
    // disabled at initialize time, so it never reaches run()).
    auto properties   = MassBalanceMock(true, 1e-5, -100).as_json_property();
    auto protocols    = NgenBmiProtocols(model, properties);
    double mass_error = 10;
    // Intermediate step: should not fire.
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    auto r = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(0, 2, time, model_name)
    );
    EXPECT_TRUE(r.has_value());
    model->Update();
    // Last step: must fire.
    model->SetValue(OUTPUT_MASS_NAME, &mass_error);
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        ),
        ProtocolError
    );
}

TEST_F(Bmi_Mass_Balance_Test, nan) {
    auto properties = MassBalanceMock(true, "NaN").as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    testing::internal::CaptureStderr();
    auto protocols     = NgenBmiProtocols(model, properties);
    std::string output = testing::internal::GetCapturedStderr();
    // std::cerr << output;
    EXPECT_THAT(
        output,
        testing::HasSubstr("Warning(Protocol)::mass_balance: tolerance value 'NaN'")
    );
    double mass_error = 10;
    model->SetValue(OUTPUT_MASS_NAME, &mass_error); //
    // Would cause an error if tolerance were a number, should only see warning in the output above
    // and this should pass without error...
    auto result = protocols.run(
        models::bmi::protocols::Protocol::MASS_BALANCE,
        make_context(0, 1, time, model_name)
    );
    EXPECT_TRUE(result.has_value()); // should pass
}

TEST_F(Bmi_Mass_Balance_Test, model_nan) {
    auto properties = MassBalanceMock(true).as_json_property();
    auto context    = make_context(0, 2, time, model_name);
    auto protocols  = NgenBmiProtocols(model, properties);
    auto result     = protocols.run(models::bmi::protocols::Protocol::MASS_BALANCE, context);
    EXPECT_TRUE(result.has_value()); // should pass
    double mass_error = std::numeric_limits<double>::quiet_NaN();
    model->SetValue(OUTPUT_MASS_NAME, &mass_error); // Force a NaN into the mass balance computation
    time = "t1";
    // should cause an error since mass balance will be NaN using this value in its computation
    ASSERT_THROW(
        (void)protocols.run(
            models::bmi::protocols::Protocol::MASS_BALANCE,
            make_context(1, 2, time, model_name)
        ),
        ProtocolError
    );
}

#endif // NGEN_BMI_CPP_LIB_TESTS_ACTIVE
