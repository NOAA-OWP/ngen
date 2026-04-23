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

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MockConfig.hpp"
#include "FileChecker.h"
#include "Bmi_Cpp_Adapter.hpp"
#include "protocols.hpp"
#include "serialization_record.hpp"

#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <vector>

using ::testing::MatchesRegex;
using models::bmi::protocols::NgenBmiProtocols;
using models::bmi::protocols::Protocol;
using models::bmi::protocols::ProtocolError;
using models::bmi::protocols::SerializationRecord;
using models::bmi::protocols::read_next_record;
using nonstd::expected_lite::expected;

#ifndef BMI_TEST_CPP_LOCAL_LIB_NAME
#ifdef __APPLE__
#define BMI_TEST_CPP_LOCAL_LIB_NAME "libtestbmicppmodel.dylib"
#else
#ifdef __GNUC__
    #define BMI_TEST_CPP_LOCAL_LIB_NAME "libtestbmicppmodel.so"
#endif
#endif
#endif

#define CREATOR_FUNC "bmi_model_create"
#define DESTROYER_FUNC "bmi_model_destroy"

using namespace models::bmi;

namespace {
    bool file_exists(const std::string& p) {
        struct stat st{};
        return ::stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
    }

    // Read every SerializationRecord in the file; return empty on missing or
    // unreadable files so tests can assert "zero records written".
    std::vector<SerializationRecord> read_all_records(const std::string& path) {
        std::vector<SerializationRecord> out;
        std::ifstream in(path, std::ios::binary);
        if (!in) return out;
        SerializationRecord scratch;
        while (read_next_record(in, scratch)) out.push_back(scratch);
        return out;
    }
}

class Bmi_Cpp_Test_Adapter_For_Serialization : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    std::string file_search(const std::vector<std::string>& parent_dir_options, const std::string& file_basename);

    std::string config_file_name_0;
    std::string lib_file_name_0;
    std::string bmi_module_type_name_0;
    std::unique_ptr<Bmi_Cpp_Adapter> adapter;
};

void Bmi_Cpp_Test_Adapter_For_Serialization::SetUp() {
    std::vector<std::string> config_path_options = {
        "test/data/bmi/test_bmi_c/",
        "./test/data/bmi/test_bmi_c/",
        "../test/data/bmi/test_bmi_c/",
        "../../test/data/bmi/test_bmi_c/",
    };
    std::string config_basename_0 = "test_bmi_c_config_0.txt";
    config_file_name_0 = file_search(config_path_options, config_basename_0);

    std::vector<std::string> lib_dir_opts = {
        "./extern/test_bmi_cpp/cmake_build/",
        "../extern/test_bmi_cpp/cmake_build/",
        "../../extern/test_bmi_cpp/cmake_build/"
    };
    lib_file_name_0 = file_search(lib_dir_opts, BMI_TEST_CPP_LOCAL_LIB_NAME);
    bmi_module_type_name_0 = "test_bmi_cpp";
    adapter = std::make_unique<Bmi_Cpp_Adapter>(
        bmi_module_type_name_0, lib_file_name_0, config_file_name_0,
        true, CREATOR_FUNC, DESTROYER_FUNC
    );
}

void Bmi_Cpp_Test_Adapter_For_Serialization::TearDown() {}

std::string
Bmi_Cpp_Test_Adapter_For_Serialization::file_search(const std::vector<std::string>& parent_dir_options,
                                                     const std::string& file_basename) {
    std::vector<std::string> name_combinations;
    for (auto& path_option : parent_dir_options)
        name_combinations.push_back(path_option + file_basename);
    return utils::FileChecker::find_first_readable(name_combinations);
}

class Bmi_Serialization_Test : public Bmi_Cpp_Test_Adapter_For_Serialization {
protected:
    void SetUp() override {
        Bmi_Cpp_Test_Adapter_For_Serialization::SetUp();
        model = std::shared_ptr<models::bmi::Bmi_Adapter>(adapter.release());
        model_name = model->GetComponentName();

        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string dir = ::testing::TempDir();
        if (!dir.empty() && dir.back() != '/') dir.push_back('/');
        path = dir + "ngen_serialization_Test_" + info->name() + ".ckpt";
        std::remove(path.c_str());
    }

    void TearDown() override {
        std::remove(path.c_str());
    }

    std::string time = "t0";
    std::string model_name;
    std::shared_ptr<models::bmi::Bmi_Adapter> model;
    std::string path;
};

// ---------------------------------------------------------------------
// Parity with mass balance: null model, default construct, unconfigured,
// disabled, missing path.
// ---------------------------------------------------------------------

TEST_F(Bmi_Serialization_Test, bad_model) {
    model = nullptr;
    auto properties = SerializationMock(path).as_json_property();
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_THAT(output, MatchesRegex(".*Disabling serialization protocol.\n.*"));

    testing::internal::CaptureStderr();
    auto result = protocols.run(Protocol::SERIALIZATION, make_context(0, 2, time, model_name));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().error_code(), models::bmi::protocols::Error::UNITIALIZED_MODEL);
    // Always drain the capture — leaving it active leaks stderr redirection
    // into the next test's SetUp (dlopen of the test model lib) and hangs.
    (void)testing::internal::GetCapturedStderr();
}

TEST_F(Bmi_Serialization_Test, default_construct) {
    auto protocols = NgenBmiProtocols();
    testing::internal::CaptureStderr();
    auto result = protocols.run(Protocol::SERIALIZATION, make_context(0, 2, time, model_name));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().error_code(), models::bmi::protocols::Error::UNITIALIZED_MODEL);
    (void)testing::internal::GetCapturedStderr();
}

TEST_F(Bmi_Serialization_Test, unconfigured) {
    auto properties = noneConfig;
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::SERIALIZATION, make_context(0, 2, time, model_name));
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
    EXPECT_FALSE(file_exists(path));
}

TEST_F(Bmi_Serialization_Test, disabled) {
    auto properties = SerializationMock(path, /*fatal*/true, /*frequency*/1, /*check*/false).as_json_property();
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::SERIALIZATION, make_context(0, 2, time, model_name));
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
    EXPECT_FALSE(file_exists(path));
}

TEST_F(Bmi_Serialization_Test, missing_path) {
    auto properties = SerializationMock::without_path(/*fatal*/true, /*frequency*/1, /*check*/true).as_json_property();
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_THAT(output, testing::HasSubstr("Warning(Protocol)::serialization: 'path' not specified"));

    auto result = protocols.run(Protocol::SERIALIZATION, make_context(0, 2, time, model_name));
    EXPECT_TRUE(result.has_value());
}

// ---------------------------------------------------------------------
// Core save behavior: one record per fired step, tagged with Context id.
// ---------------------------------------------------------------------

TEST_F(Bmi_Serialization_Test, check_writes_record) {
    auto properties = SerializationMock(path).as_json_property();
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);

    // Use numeric-string timestamps so the protocol's string->int64 parse
    // has something to round-trip through to the record field. Non-numeric
    // Context::timestamp values are valid but land as 0 on disk — that path
    // is exercised by other tests below that don't assert on the numeric
    // timestamp field.
    auto result = protocols.run(Protocol::SERIALIZATION, make_context(0, 2, "0", model_name));
    EXPECT_TRUE(result.has_value());
    model->Update();
    result = protocols.run(Protocol::SERIALIZATION, make_context(1, 2, "3600", model_name));
    EXPECT_TRUE(result.has_value());

    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
    auto records = read_all_records(path);
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].id, model_name);
    EXPECT_EQ(records[0].time_step, 0);
    EXPECT_EQ(records[0].timestamp, int64_t{0});
    EXPECT_EQ(records[1].id, model_name);
    EXPECT_EQ(records[1].time_step, 1);
    EXPECT_EQ(records[1].timestamp, int64_t{3600});
    // test_bmi_cpp serializes time + 4 doubles == 40 bytes of payload
    EXPECT_EQ(records[0].payload.size(), 40u);
    EXPECT_EQ(records[1].payload.size(), 40u);
}

// The explicit motivation for the single-file design: two entities must be
// able to share one file and each remain recoverable by id.
TEST_F(Bmi_Serialization_Test, multiple_entities_share_one_file) {
    auto properties = SerializationMock(path).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    // Fire three times with different ids; each record must be tagged by Context::id.
    protocols.run(Protocol::SERIALIZATION, make_context(0, 2, "t0", "cat-A"));
    protocols.run(Protocol::SERIALIZATION, make_context(0, 2, "t0", "cat-B"));
    model->Update();
    protocols.run(Protocol::SERIALIZATION, make_context(1, 2, "t1", "cat-A"));

    auto records = read_all_records(path);
    ASSERT_EQ(records.size(), 3u);
    EXPECT_EQ(records[0].id, "cat-A"); EXPECT_EQ(records[0].time_step, 0);
    EXPECT_EQ(records[1].id, "cat-B"); EXPECT_EQ(records[1].time_step, 0);
    EXPECT_EQ(records[2].id, "cat-A"); EXPECT_EQ(records[2].time_step, 1);
}

// ---------------------------------------------------------------------
// Frequency gating (mirrors mass balance).
// ---------------------------------------------------------------------

TEST_F(Bmi_Serialization_Test, frequency) {
    auto properties = SerializationMock(path, /*fatal*/true, /*frequency*/2).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    protocols.run(Protocol::SERIALIZATION, make_context(0, 4, "t0", model_name));  // fires
    model->Update();
    protocols.run(Protocol::SERIALIZATION, make_context(1, 4, "t1", model_name));  // skipped
    model->Update();
    protocols.run(Protocol::SERIALIZATION, make_context(2, 4, "t2", model_name));  // fires

    auto records = read_all_records(path);
    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].time_step, 0);
    EXPECT_EQ(records[1].time_step, 2);
}

TEST_F(Bmi_Serialization_Test, frequency_zero) {
    auto properties = SerializationMock(path, /*fatal*/true, /*frequency*/0).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    protocols.run(Protocol::SERIALIZATION, make_context(0, 2, "t0", model_name));
    EXPECT_FALSE(file_exists(path));
}

TEST_F(Bmi_Serialization_Test, frequency_end) {
    auto properties = SerializationMock(path, /*fatal*/true, /*frequency*/-1).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    protocols.run(Protocol::SERIALIZATION, make_context(0, 2, "t0", model_name));
    model->Update();
    protocols.run(Protocol::SERIALIZATION, make_context(1, 2, "t1", model_name));
    model->Update();
    protocols.run(Protocol::SERIALIZATION, make_context(2, 2, "t2", model_name));

    auto records = read_all_records(path);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].time_step, 2);
}

// ---------------------------------------------------------------------
// I/O error handling.
// ---------------------------------------------------------------------

TEST_F(Bmi_Serialization_Test, write_error_warns) {
    std::string bad_dir_path = path + "/nonexistent_subdir/file.ckpt";
    auto properties = SerializationMock(bad_dir_path, /*fatal*/false).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    auto result = protocols.run(Protocol::SERIALIZATION, make_context(0, 2, "t0", model_name));
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().to_string(), testing::HasSubstr("Warning(Protocol)::serialization:"));
}

TEST_F(Bmi_Serialization_Test, write_error_throws) {
    std::string bad_dir_path = path + "/nonexistent_subdir/file.ckpt";
    auto properties = SerializationMock(bad_dir_path, /*fatal*/true).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    ASSERT_THROW(
        protocols.run(Protocol::SERIALIZATION, make_context(0, 2, "t0", model_name)),
        ProtocolError
    );
}

#endif  // NGEN_BMI_CPP_LIB_TESTS_ACTIVE
