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
End-to-end BMI serialization protocol test against the C reference
model. Sibling tests: cpp_serialization_Test.cpp /
cpp_deserialization_Test.cpp exercise the same protocol via the C++
test model; fortran_serialization_Test.cpp and py_serialization_Test.cpp
cover the Fortran and Python analogs. This file is the C analog.
*/
#ifdef NGEN_BMI_C_LIB_TESTS_ACTIVE

#include "Bmi_C_Adapter.hpp"
#include "FileChecker.h"
#include "MockConfig.hpp"
#include "protocols.hpp"
#include "serialization.hpp"
#include "serialization_record.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sys/stat.h>
#include <vector>

#ifndef BMI_TEST_C_LOCAL_LIB_NAME
#ifdef __APPLE__
#define BMI_TEST_C_LOCAL_LIB_NAME "libtestbmicmodel.dylib"
#else
#ifdef __GNUC__
#define BMI_TEST_C_LOCAL_LIB_NAME "libtestbmicmodel.so"
#endif
#endif
#endif

#define REGISTRATION_FUNC "register_bmi"

using models::bmi::Bmi_C_Adapter;
using models::bmi::protocols::NgenBmiProtocols;
using models::bmi::protocols::Protocol;
using models::bmi::protocols::ProtocolError;
using models::bmi::protocols::read_next_record;
using models::bmi::protocols::SerializationRecord;

namespace {
bool file_exists(const std::string& p) {
    struct stat st{};
    return ::stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::vector<SerializationRecord> read_all_records(const std::string& path) {
    std::vector<SerializationRecord> out;
    std::ifstream in(path, std::ios::binary);
    if (!in) return out;
    SerializationRecord scratch;
    for (;;) {
        auto r = read_next_record(in, scratch);
        if (!r) break; // error arm — treat as end of readable content
        if (r.value() == models::bmi::protocols::Status::Eof) break;
        out.push_back(scratch);
    }
    return out;
}

std::string file_search(const std::vector<std::string>& dirs, const std::string& basename) {
    std::vector<std::string> combos;
    for (auto& d : dirs) combos.push_back(d + basename);
    return utils::FileChecker::find_first_readable(combos);
}
} // namespace

class Bmi_C_Serialization_Test : public ::testing::Test {
  protected:
    void SetUp() override {
        std::vector<std::string> cfg_paths = {
            "test/data/bmi/test_bmi_c/",
            "./test/data/bmi/test_bmi_c/",
            "../test/data/bmi/test_bmi_c/",
            "../../test/data/bmi/test_bmi_c/",
        };
        config_file_name = file_search(cfg_paths, "test_bmi_c_config_0.txt");

        std::vector<std::string> lib_dirs = {
            "./extern/test_bmi_c/cmake_build/",
            "../extern/test_bmi_c/cmake_build/",
            "../../extern/test_bmi_c/cmake_build/"
        };
        lib_file_name    = file_search(lib_dirs, BMI_TEST_C_LOCAL_LIB_NAME);
        module_type_name = "test_bmi_c";

        auto adapter = std::make_shared<Bmi_C_Adapter>(
            module_type_name,
            lib_file_name,
            config_file_name,
            /*allow_exceed_end_time*/ true,
            REGISTRATION_FUNC
        );
        model_name = adapter->GetComponentName();
        model      = std::static_pointer_cast<models::bmi::Bmi_Adapter>(adapter);

        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string dir  = ::testing::TempDir();
        if (!dir.empty() && dir.back() != '/') dir.push_back('/');
        path = dir + "ngen_c_serialization_Test_" + info->name() + ".ckpt";
        std::remove(path.c_str());
    }

    void TearDown() override {
        std::remove(path.c_str());
    }

    std::string config_file_name;
    std::string lib_file_name;
    std::string module_type_name;
    std::string model_name;
    std::shared_ptr<models::bmi::Bmi_Adapter> model;
    std::string path;
};

// ---------------------------------------------------------------------
// Support detection — conforming C model passes the metadata probe.
// ---------------------------------------------------------------------

TEST_F(Bmi_C_Serialization_Test, check_support_passes) {
    // Default fatal=true; any units mismatch in the C model's
    // `serialization_var_units` array would escalate to PROTOCOL_ERROR
    // at construction. Clean construction means check_support passed.
    auto properties = SerializationMock(path).as_json_property();
    EXPECT_NO_THROW({
        auto protocols = NgenBmiProtocols(model, properties);
        (void)protocols;
    });
}

// ---------------------------------------------------------------------
// Save side — one run() call writes one record tagged with Context::id.
// ---------------------------------------------------------------------

TEST_F(Bmi_C_Serialization_Test, save_writes_record) {
    auto properties = SerializationMock(path).as_json_property();
    auto protocols  = NgenBmiProtocols(model, properties);

    auto result = protocols.run(Protocol::SERIALIZATION, make_context(0, 2, "0", "c-cat-1"));
    EXPECT_TRUE(result.has_value());

    auto records = read_all_records(path);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].id, "c-cat-1");
    EXPECT_EQ(records[0].time_step, 0);
    // The C test model's layout packs current_model_time (8) +
    // input_var_1 (8) + input_var_2 (8) + output_var_1 (8) +
    // output_var_2 (8) = 40 bytes; see SERIALIZED_STATE_BYTES in
    // bmi_test_bmi_c.c.
    EXPECT_EQ(records[0].payload.size(), 40u);
}

// ---------------------------------------------------------------------
// Integration: save -> diverge -> restore round trip. Proves that the
// C adapter's value-marshalling path round-trips the model state bytes
// faithfully across the language boundary for both directions.
// ---------------------------------------------------------------------

TEST_F(Bmi_C_Serialization_Test, save_restore_roundtrip) {
    auto properties =
        DeserializationMock(path, /*step*/ "latest").with_save(/*frequency*/ 1).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    // Prime with distinct inputs that the serialized layout will
    // carry. The C test model exposes two double inputs.
    double input1 = 17.75;
    double input2 = 3.125;
    model->SetValue("INPUT_VAR_1", &input1);
    model->SetValue("INPUT_VAR_2", &input2);
    model->Update();

    const double t_ref = model->GetCurrentTime();

    // Save at step 1.
    auto save_r =
        protocols.run(Protocol::SERIALIZATION, make_context(1, 3, "3600", "c-cat-1"));
    ASSERT_TRUE(save_r.has_value());
    ASSERT_TRUE(file_exists(path));

    // Explicit divergence: overwrite the serialized inputs with
    // clearly-different values AND advance the clock. A bare Update()
    // would only move current_model_time, leaving the inputs
    // untouched, which means a silently-broken restore could pass the
    // post-restore input checks simply because the inputs were never
    // actually perturbed.
    double bogus1 = -777.0;
    double bogus2 = -8.5;
    model->SetValue("INPUT_VAR_1", &bogus1);
    model->SetValue("INPUT_VAR_2", &bogus2);
    model->Update();
    model->Update();
    ASSERT_NE(model->GetCurrentTime(), t_ref);

    {
        double i1_diverged = 0.0;
        double i2_diverged = 0.0;
        model->GetValue("INPUT_VAR_1", &i1_diverged);
        model->GetValue("INPUT_VAR_2", &i2_diverged);
        ASSERT_EQ(i1_diverged, bogus1) << "pre-restore divergence check (INPUT_VAR_1)";
        ASSERT_EQ(i2_diverged, bogus2) << "pre-restore divergence check (INPUT_VAR_2)";
    }

    // Restore.
    auto restore_r =
        protocols.run(Protocol::DESERIALIZATION, make_context(0, 0, "restore", "c-cat-1"));
    ASSERT_TRUE(restore_r.has_value());

    // Verify every serialized field round-tripped.
    EXPECT_EQ(model->GetCurrentTime(), t_ref);
    double i1 = 0.0;
    double i2 = 0.0;
    model->GetValue("INPUT_VAR_1", &i1);
    model->GetValue("INPUT_VAR_2", &i2);
    EXPECT_EQ(i1, input1);
    EXPECT_EQ(i2, input2);
}

// ---------------------------------------------------------------------
// 32 bit ngen::serialization_size support. A model may declare its SIZE
// variable as a narrower int (itemsize = nbytes = 4) and rely on the
// framework's zero-init int64_t plus little-endian byte layout
// to receive values up to INT32_MAX correctly. 
// This is only supported on little-endian hosts.
// ---------------------------------------------------------------------

TEST_F(Bmi_C_Serialization_Test, simple_path_narrow_int_size_roundtrips) {
    const int64_t written  = INT32_MAX;
    int64_t       readback = 0;

    model->SetValue("test::serialization_32bit", const_cast<int64_t*>(&written));
    model->GetValue("test::serialization_32bit", &readback);

    EXPECT_EQ(readback, written)
        << "32 bit int, with BMI itemsize = nbytes "
           "= sizeof(int) did not round-trip on this host:" << std::endl <<
           "SetValue: "<< written << ", GetValue returned: " << readback <<  std::endl <<
           "Only models with ngen::serialization_size declared as `int64_t` "
           "and reporting itemsize = nbytes = sizeof(int64_t) can properly "
           "serialize and deserialize";
}

#endif // NGEN_BMI_C_LIB_TESTS_ACTIVE
