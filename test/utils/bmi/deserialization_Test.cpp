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
Restore-side unit tests. The happy-path tests prepare a checkpoint file by
hand (using the record helpers directly), then run the deserialization
protocol and assert that SetValue(ngen::serialization_state, ...) flowed.
This keeps the restore tests independent of the save protocol.

One integration test at the end runs save -> restore end-to-end on the
same path to cover the full feedback loop.
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
using models::bmi::protocols::write_record;
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

    // Helper: produce a payload that the test_bmi_cpp model will accept via
    // SetValue(ngen::serialization_state, ...). The model's on-disk layout is:
    //     [current_model_time: double][input_var_1: double][input_var_2: double]
    //     [output_var_1: double][output_var_2: double]
    // All packed, native-endian, 40 bytes total. This mirrors the create
    // routine in extern/test_bmi_cpp/include/test_bmi_cpp.hpp.
    std::vector<char> make_payload(double t, double i1, double i2, double o1, double o2) {
        std::vector<char> buf(5 * sizeof(double));
        char* p = buf.data();
        std::memcpy(p,                     &t,  sizeof(double));
        std::memcpy(p + 1 * sizeof(double), &i1, sizeof(double));
        std::memcpy(p + 2 * sizeof(double), &i2, sizeof(double));
        std::memcpy(p + 3 * sizeof(double), &o1, sizeof(double));
        std::memcpy(p + 4 * sizeof(double), &o2, sizeof(double));
        return buf;
    }

    // Append a pre-built record to the checkpoint file at @p path.
    void append_record(const std::string& path, const SerializationRecord& rec) {
        std::ofstream out(path, std::ios::binary | std::ios::app);
        ASSERT_TRUE(out.good());
        write_record(out, rec);
    }
}

class Bmi_Cpp_Test_Adapter_For_Deserialization : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    std::string file_search(const std::vector<std::string>& dirs, const std::string& basename);

    std::string config_file_name_0;
    std::string lib_file_name_0;
    std::string bmi_module_type_name_0;
    std::unique_ptr<Bmi_Cpp_Adapter> adapter;
};

void Bmi_Cpp_Test_Adapter_For_Deserialization::SetUp() {
    std::vector<std::string> cfg_paths = {
        "test/data/bmi/test_bmi_c/",
        "./test/data/bmi/test_bmi_c/",
        "../test/data/bmi/test_bmi_c/",
        "../../test/data/bmi/test_bmi_c/",
    };
    config_file_name_0 = file_search(cfg_paths, "test_bmi_c_config_0.txt");

    std::vector<std::string> lib_dirs = {
        "./extern/test_bmi_cpp/cmake_build/",
        "../extern/test_bmi_cpp/cmake_build/",
        "../../extern/test_bmi_cpp/cmake_build/"
    };
    lib_file_name_0 = file_search(lib_dirs, BMI_TEST_CPP_LOCAL_LIB_NAME);
    bmi_module_type_name_0 = "test_bmi_cpp";
    adapter = std::make_unique<Bmi_Cpp_Adapter>(
        bmi_module_type_name_0, lib_file_name_0, config_file_name_0,
        true, CREATOR_FUNC, DESTROYER_FUNC
    );
}
void Bmi_Cpp_Test_Adapter_For_Deserialization::TearDown() {}

std::string
Bmi_Cpp_Test_Adapter_For_Deserialization::file_search(const std::vector<std::string>& dirs,
                                                      const std::string& basename) {
    std::vector<std::string> combos;
    for (auto& d : dirs) combos.push_back(d + basename);
    return utils::FileChecker::find_first_readable(combos);
}

class Bmi_Deserialization_Test : public Bmi_Cpp_Test_Adapter_For_Deserialization {
protected:
    void SetUp() override {
        Bmi_Cpp_Test_Adapter_For_Deserialization::SetUp();
        model = std::shared_ptr<models::bmi::Bmi_Adapter>(adapter.release());
        model_name = model->GetComponentName();

        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string dir = ::testing::TempDir();
        if (!dir.empty() && dir.back() != '/') dir.push_back('/');
        path = dir + "ngen_deserialization_Test_" + info->name() + ".ckpt";
        std::remove(path.c_str());
    }
    void TearDown() override {
        std::remove(path.c_str());
    }

    // Read the model's five serialized scalars via BMI so tests can assert
    // on model state after restore.
    void snapshot(double& t, double& i1, double& i2, double& o1, double& o2) {
        t = model->GetCurrentTime();
        model->GetValue("INPUT_VAR_1",  &i1);
        model->GetValue("INPUT_VAR_2",  &i2);
        model->GetValue("OUTPUT_VAR_1", &o1);
        model->GetValue("OUTPUT_VAR_2", &o2);
    }

    std::string model_name;
    std::shared_ptr<models::bmi::Bmi_Adapter> model;
    std::string path;
};

// ---------------------------------------------------------------------
// Configuration-level parity tests.
// ---------------------------------------------------------------------

TEST_F(Bmi_Deserialization_Test, bad_model) {
    model = nullptr;
    auto properties = DeserializationMock(path).as_json_property();
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().error_code(), models::bmi::protocols::Error::UNITIALIZED_MODEL);
    (void)testing::internal::GetCapturedStderr();  // drain to avoid cross-test bleed
}

TEST_F(Bmi_Deserialization_Test, default_construct) {
    auto protocols = NgenBmiProtocols();
    testing::internal::CaptureStderr();
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().error_code(), models::bmi::protocols::Error::UNITIALIZED_MODEL);
    (void)testing::internal::GetCapturedStderr();
}

TEST_F(Bmi_Deserialization_Test, unconfigured) {
    auto properties = noneConfig;
    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, properties);
    // No config -> protocol is silently disabled regardless of file state.
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name));
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
}

TEST_F(Bmi_Deserialization_Test, disabled) {
    // restore sub-block present but check=false must be a no-op even when
    // records exist for this id.
    append_record(path, SerializationRecord{model_name, 0, int64_t{0}, make_payload(0, 1, 2, 3, 4)});
    boost::property_tree::ptree ptree;
    boost::property_tree::ptree top;
    top.put("path", path);
    boost::property_tree::ptree restore;
    restore.put("check", false);
    top.add_child("restore", restore);
    ptree.add_child("serialization", top);
    auto props = geojson::JSONProperty("serialization", ptree).get_values();

    auto protocols = NgenBmiProtocols(model, props);
    double t_before, i1, i2, o1, o2;
    snapshot(t_before, i1, i2, o1, o2);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name));
    EXPECT_TRUE(result.has_value());

    // Disabled protocol must not have touched the model.
    double t_after;
    snapshot(t_after, i1, i2, o1, o2);
    EXPECT_EQ(t_after, t_before);
}

TEST_F(Bmi_Deserialization_Test, missing_path) {
    // restore block without top-level path -> warn and disable.
    boost::property_tree::ptree ptree;
    boost::property_tree::ptree top;
    boost::property_tree::ptree restore;
    restore.put("check", true);
    top.add_child("restore", restore);
    ptree.add_child("serialization", top);
    auto props = geojson::JSONProperty("serialization", ptree).get_values();

    testing::internal::CaptureStderr();
    auto protocols = NgenBmiProtocols(model, props);
    std::string output = testing::internal::GetCapturedStderr();
    EXPECT_THAT(output, testing::HasSubstr("Warning(Protocol)::deserialization: 'path' not specified"));
}

// ---------------------------------------------------------------------
// Restore semantics.
// ---------------------------------------------------------------------

TEST_F(Bmi_Deserialization_Test, restore_fixed_step) {
    // Pre-populate the file with two records for this id at different steps,
    // plus a distractor record for a different id.
    append_record(path, SerializationRecord{model_name, 0, int64_t{0},    make_payload(0.0,   1.0, 2.0, 3.0, 4.0)});
    append_record(path, SerializationRecord{"other",    0, int64_t{0},    make_payload(999.0, 9.0, 9.0, 9.0, 9.0)});
    append_record(path, SerializationRecord{model_name, 5, int64_t{300},  make_payload(500.0, 5.0, 6.0, 7.0, 8.0)});

    // Ask for step=0. The distractor must be ignored; the step=5 record must not win.
    auto properties = DeserializationMock(path, /*step*/"0").as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name));
    ASSERT_TRUE(result.has_value());

    double t, i1, i2, o1, o2;
    snapshot(t, i1, i2, o1, o2);
    EXPECT_EQ(t, 0.0);
    EXPECT_EQ(i1, 1.0);
    EXPECT_EQ(i2, 2.0);
    EXPECT_EQ(o1, 3.0);
    EXPECT_EQ(o2, 4.0);
}

TEST_F(Bmi_Deserialization_Test, restore_latest_step) {
    // Two records for this id at different steps -> "latest" must pick the
    // higher time_step (not order of appearance).
    append_record(path, SerializationRecord{model_name, 5, int64_t{300}, make_payload(500.0, 5.0, 6.0, 7.0, 8.0)});
    append_record(path, SerializationRecord{model_name, 0, int64_t{0},   make_payload(  0.0, 1.0, 2.0, 3.0, 4.0)});

    auto properties = DeserializationMock(path, /*step*/"latest").as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name));
    ASSERT_TRUE(result.has_value());

    double t, i1, i2, o1, o2;
    snapshot(t, i1, i2, o1, o2);
    EXPECT_EQ(t, 500.0);
    EXPECT_EQ(i1,  5.0);
}

TEST_F(Bmi_Deserialization_Test, missing_id_warns) {
    // File exists but nothing matches this id.
    append_record(path, SerializationRecord{"some-other-id", 0, int64_t{0}, make_payload(0, 1, 2, 3, 4)});

    auto properties = DeserializationMock(path, /*step*/"latest", /*fatal*/false).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name));
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().to_string(), testing::HasSubstr("Warning(Protocol)::deserialization:"));
    EXPECT_THAT(result.error().to_string(), testing::HasSubstr("no matching record"));
}

TEST_F(Bmi_Deserialization_Test, missing_id_throws) {
    append_record(path, SerializationRecord{"some-other-id", 0, int64_t{0}, make_payload(0, 1, 2, 3, 4)});

    auto properties = DeserializationMock(path, /*step*/"latest", /*fatal*/true).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);
    ASSERT_THROW(
        protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name)),
        ProtocolError
    );
}

TEST_F(Bmi_Deserialization_Test, missing_step_warns) {
    // Id matches, but not at the requested step.
    append_record(path, SerializationRecord{model_name, 0, int64_t{0}, make_payload(0, 1, 2, 3, 4)});

    auto properties = DeserializationMock(path, /*step*/"5", /*fatal*/false).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name));
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().to_string(),
                testing::HasSubstr("no matching record for id '" + model_name + "' at step 5"));
}

TEST_F(Bmi_Deserialization_Test, missing_file_warns) {
    // File doesn't exist at all.
    ASSERT_FALSE(file_exists(path));
    auto properties = DeserializationMock(path, /*step*/"latest", /*fatal*/false).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 2, "t0", model_name));
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().to_string(), testing::HasSubstr("no matching record"));
}

// ---------------------------------------------------------------------
// Timestamp-based restore (v2 format).
// ---------------------------------------------------------------------

TEST_F(Bmi_Deserialization_Test, restore_by_timestamp) {
    // Three records for this id at distinct timestamps. Config picks one by
    // exact timestamp match (int64 seconds-since-epoch-style), independent
    // of time_step.
    append_record(path, SerializationRecord{model_name, 0, int64_t{1448928000}, make_payload(  0.0, 1.0, 2.0, 3.0, 4.0)});
    append_record(path, SerializationRecord{model_name, 1, int64_t{1448931600}, make_payload(100.0, 11., 12., 13., 14.)});
    append_record(path, SerializationRecord{model_name, 2, int64_t{1448935200}, make_payload(200.0, 21., 22., 23., 24.)});

    auto properties = DeserializationMock::by_timestamp(path, "1448931600").as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 0, "unused", model_name));
    ASSERT_TRUE(result.has_value());

    double t, i1, i2, o1, o2;
    snapshot(t, i1, i2, o1, o2);
    EXPECT_EQ(t,  100.0);
    EXPECT_EQ(i1,  11.0);
}

TEST_F(Bmi_Deserialization_Test, restore_timestamp_wins_over_step) {
    // Two records at different (step, timestamp) pairs. Config specifies both,
    // timestamp wins — we must land on the timestamp-matched record, not the
    // step-matched one.
    append_record(path, SerializationRecord{model_name, 5, int64_t{300}, make_payload(500.0, 5.0, 6.0, 7.0, 8.0)});
    append_record(path, SerializationRecord{model_name, 0, int64_t{0},   make_payload(  0.0, 1.0, 2.0, 3.0, 4.0)});

    // Asking for step=5 AND timestamp=0; the timestamp's record (step 0) must win.
    auto properties = DeserializationMock::both(path, /*step*/"5", /*timestamp*/"0").as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 0, "unused", model_name));
    ASSERT_TRUE(result.has_value());

    double t, i1, i2, o1, o2;
    snapshot(t, i1, i2, o1, o2);
    EXPECT_EQ(t, 0.0);
    EXPECT_EQ(i1, 1.0);
}

TEST_F(Bmi_Deserialization_Test, missing_timestamp_warns) {
    // Id matches but no record has the requested timestamp.
    append_record(path, SerializationRecord{model_name, 0, int64_t{1448928000}, make_payload(0, 1, 2, 3, 4)});

    auto properties = DeserializationMock::by_timestamp(path, "1577836800", /*fatal*/false).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);
    auto result = protocols.run(Protocol::DESERIALIZATION, make_context(0, 0, "unused", model_name));
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().to_string(),
                testing::HasSubstr("at timestamp '1577836800'"));
}

// ---------------------------------------------------------------------
// Integration: save -> restore round trip, one configuration, one file.
// ---------------------------------------------------------------------

TEST_F(Bmi_Deserialization_Test, save_restore_roundtrip) {
    // Single config drives both save and restore; both sub-blocks share the path.
    auto properties = DeserializationMock(path, /*step*/"latest").with_save(/*frequency*/1).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    // Prime model with distinct scalars, step once, then capture the state.
    double input1 = 7.5, input2 = 3.25;
    model->SetValue("INPUT_VAR_1", &input1);
    model->SetValue("INPUT_VAR_2", &input2);
    model->Update();

    double t_ref, i1_ref, i2_ref, o1_ref, o2_ref;
    snapshot(t_ref, i1_ref, i2_ref, o1_ref, o2_ref);

    // Save at step 1.
    auto save_r = protocols.run(Protocol::SERIALIZATION, make_context(1, 3, "t1", model_name));
    ASSERT_TRUE(save_r.has_value());
    ASSERT_TRUE(file_exists(path));

    // Advance the model so its state diverges.
    model->Update();
    model->Update();
    ASSERT_NE(model->GetCurrentTime(), t_ref);

    // Restore (step=latest finds the only record we wrote: step 1).
    auto restore_r = protocols.run(Protocol::DESERIALIZATION, make_context(0, 0, "restore", model_name));
    ASSERT_TRUE(restore_r.has_value());

    double t, i1, i2, o1, o2;
    snapshot(t, i1, i2, o1, o2);
    EXPECT_EQ(t,  t_ref);
    EXPECT_EQ(i1, i1_ref);
    EXPECT_EQ(i2, i2_ref);
    EXPECT_EQ(o1, o1_ref);
    EXPECT_EQ(o2, o2_ref);
}

// Same integration as above but restore is keyed by timestamp — verifies
// the save protocol actually persisted ctx.timestamp and the restore side
// found the record via that field rather than a step match. Uses a
// numeric timestamp string so the string->int64 parse on both sides
// yields a matching on-disk value.
TEST_F(Bmi_Deserialization_Test, save_restore_roundtrip_by_timestamp) {
    const std::string save_timestamp = "1448931600";  // 2015-12-01 01:00:00 UTC
    auto properties = DeserializationMock::by_timestamp(path, save_timestamp).with_save(/*frequency*/1).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    double input1 = 4.0, input2 = 2.0;
    model->SetValue("INPUT_VAR_1", &input1);
    model->SetValue("INPUT_VAR_2", &input2);
    model->Update();

    double t_ref, i1_ref, i2_ref, o1_ref, o2_ref;
    snapshot(t_ref, i1_ref, i2_ref, o1_ref, o2_ref);

    // Save with the timestamp we'll later restore by.
    auto save_r = protocols.run(Protocol::SERIALIZATION, make_context(1, 3, save_timestamp, model_name));
    ASSERT_TRUE(save_r.has_value());

    // Advance, then restore — the record was keyed by timestamp, not step.
    model->Update();
    model->Update();
    ASSERT_NE(model->GetCurrentTime(), t_ref);

    auto restore_r = protocols.run(Protocol::DESERIALIZATION, make_context(0, 0, "unused", model_name));
    ASSERT_TRUE(restore_r.has_value());

    double t, i1, i2, o1, o2;
    snapshot(t, i1, i2, o1, o2);
    EXPECT_EQ(t,  t_ref);
    EXPECT_EQ(i1, i1_ref);
    EXPECT_EQ(i2, i2_ref);
}

#endif  // NGEN_BMI_CPP_LIB_TESTS_ACTIVE
