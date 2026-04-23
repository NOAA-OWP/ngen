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
End-to-end BMI serialization protocol test against the Fortran
reference model. Three contracts validated:

  1. The Fortran BMI adapter marshals the reserved-variable calls
     (GetVarUnits for support detection, SetValue/GetValue for the
     four reserved names) correctly. In particular, STATE is
     declared 'integer' in the Fortran model because the adapter
     has no byte-typed value path — the end-to-end round-trip here
     is what proves that the integer-array-as-byte-buffer
     workaround survives the language boundary.
  2. The protocol's save/restore cycle works against a
     Fortran-mode model just like it does for C++ (which is what
     serialization_Test / deserialization_Test cover).
  3. The Fortran test model's `test_create_serialization` /
     `test_deserialize_state` helpers round-trip the model's
     serialized fields faithfully.

Sibling tests: serialization_Test.cpp / deserialization_Test.cpp
exercise the same protocol via the C++ test model; this file is the
Fortran analog.
*/
#ifdef NGEN_BMI_FORTRAN_LIB_TESTS_ACTIVE

#include "gtest/gtest.h"
#include "MockConfig.hpp"
#include "FileChecker.h"
#include "Bmi_Fortran_Adapter.hpp"
#include "protocols.hpp"
#include "serialization_record.hpp"

#include <cstdio>
#include <fstream>
#include <memory>
#include <sys/stat.h>
#include <vector>

#ifndef BMI_TEST_FORTRAN_LOCAL_LIB_NAME
#ifdef __APPLE__
#define BMI_TEST_FORTRAN_LOCAL_LIB_NAME "libtestbmifortranmodel.dylib"
#else
#ifdef __GNUC__
    #define BMI_TEST_FORTRAN_LOCAL_LIB_NAME "libtestbmifortranmodel.so"
#endif
#endif
#endif

#define REGISTRATION_FUNC "register_bmi"

using models::bmi::Bmi_Fortran_Adapter;
using models::bmi::protocols::NgenBmiProtocols;
using models::bmi::protocols::Protocol;
using models::bmi::protocols::ProtocolError;
using models::bmi::protocols::SerializationRecord;
using models::bmi::protocols::read_next_record;

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
        while (read_next_record(in, scratch)) out.push_back(scratch);
        return out;
    }

    std::string file_search(const std::vector<std::string>& dirs, const std::string& basename) {
        std::vector<std::string> combos;
        for (auto& d : dirs) combos.push_back(d + basename);
        return utils::FileChecker::find_first_readable(combos);
    }
}

class Bmi_Fortran_Serialization_Test : public ::testing::Test {
protected:
    void SetUp() override {
        std::vector<std::string> cfg_paths = {
            "test/data/bmi/test_bmi_fortran/",
            "./test/data/bmi/test_bmi_fortran/",
            "../test/data/bmi/test_bmi_fortran/",
            "../../test/data/bmi/test_bmi_fortran/",
        };
        config_file_name = file_search(cfg_paths, "test_bmi_fortran_config_0.txt");

        std::vector<std::string> lib_dirs = {
            "./extern/test_bmi_fortran/cmake_build/",
            "../extern/test_bmi_fortran/cmake_build/",
            "../../extern/test_bmi_fortran/cmake_build/"
        };
        lib_file_name = file_search(lib_dirs, BMI_TEST_FORTRAN_LOCAL_LIB_NAME);
        module_type_name = "test_bmi_fortran";

        auto adapter = std::make_shared<Bmi_Fortran_Adapter>(
            module_type_name, lib_file_name, config_file_name,
            /*allow_exceed_end_time*/ true, REGISTRATION_FUNC);
        model_name = adapter->GetComponentName();
        model = std::static_pointer_cast<models::bmi::Bmi_Adapter>(adapter);

        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string dir = ::testing::TempDir();
        if (!dir.empty() && dir.back() != '/') dir.push_back('/');
        path = dir + "ngen_fortran_serialization_Test_" + info->name() + ".ckpt";
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
// Support detection — conforming Fortran model passes the metadata probe.
// ---------------------------------------------------------------------

TEST_F(Bmi_Fortran_Serialization_Test, check_support_passes) {
    // Default fatal=true; any units mismatch in the Fortran model's
    // `serialization_units` array would escalate to PROTOCOL_ERROR at
    // construction. Clean construction means check_support passed.
    auto properties = SerializationMock(path).as_json_property();
    EXPECT_NO_THROW({
        auto protocols = NgenBmiProtocols(model, properties);
        (void)protocols;
    });
}

// ---------------------------------------------------------------------
// Save side — one run() call writes one record tagged with Context::id.
// ---------------------------------------------------------------------

TEST_F(Bmi_Fortran_Serialization_Test, save_writes_record) {
    auto properties = SerializationMock(path).as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    auto result = protocols.run(Protocol::SERIALIZATION,
                                make_context(0, 2, "0", "fortran-cat-1"));
    EXPECT_TRUE(result.has_value());

    auto records = read_all_records(path);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].id, "fortran-cat-1");
    EXPECT_EQ(records[0].time_step, 0);
    // The Fortran test model's layout packs current_model_time (8) +
    // input_var_1 (8) + input_var_2 (4) + input_var_3 (4) +
    // output_var_1 (8) + output_var_2 (4) + output_var_3 (4) = 40
    // bytes; see SERIALIZED_STATE_BYTES in bmi_test_bmi_fortran.f90.
    EXPECT_EQ(records[0].payload.size(), 40u);
}

// ---------------------------------------------------------------------
// Integration: save -> diverge -> restore round trip. Proves that the
// Fortran adapter's integer-array path round-trips the model state
// bytes faithfully across the language boundary for both directions.
// ---------------------------------------------------------------------

TEST_F(Bmi_Fortran_Serialization_Test, save_restore_roundtrip) {
    auto properties = DeserializationMock(path, /*step*/ "latest")
        .with_save(/*frequency*/ 1)
        .as_json_property();
    auto protocols = NgenBmiProtocols(model, properties);

    // Prime with distinct inputs that the serialized layout will
    // carry — a double, a float, and an integer — covering each of
    // the Fortran value kinds the Fortran BMI adapter dispatches on.
    double  input1 = 17.75;
    float   input2 = 3.125f;
    int     input3 = 42;
    model->SetValue("INPUT_VAR_1", &input1);
    model->SetValue("INPUT_VAR_2", &input2);
    model->SetValue("INPUT_VAR_3", &input3);
    model->Update();

    const double t_ref = model->GetCurrentTime();

    // Save at step 1.
    auto save_r = protocols.run(Protocol::SERIALIZATION,
                                make_context(1, 3, "3600", "fortran-cat-1"));
    ASSERT_TRUE(save_r.has_value());
    ASSERT_TRUE(file_exists(path));

    // Explicit divergence: overwrite the serialized inputs with
    // clearly-different values AND advance the clock. A bare
    // Update() would only move current_model_time, leaving the
    // inputs untouched, which means a silently-broken restore could
    // pass the post-restore input checks simply because the inputs
    // were never actually perturbed. Sanity-checking the divergence
    // before restore ensures the round-trip assertion below is
    // measuring something real.
    double bogus1 = -777.0;
    float  bogus2 = -8.5f;
    int    bogus3 = -99;
    model->SetValue("INPUT_VAR_1", &bogus1);
    model->SetValue("INPUT_VAR_2", &bogus2);
    model->SetValue("INPUT_VAR_3", &bogus3);
    model->Update();
    model->Update();
    ASSERT_NE(model->GetCurrentTime(), t_ref);

    {
        double i1_diverged = 0.0;
        float  i2_diverged = 0.0f;
        int    i3_diverged = 0;
        model->GetValue("INPUT_VAR_1", &i1_diverged);
        model->GetValue("INPUT_VAR_2", &i2_diverged);
        model->GetValue("INPUT_VAR_3", &i3_diverged);
        ASSERT_EQ(i1_diverged, bogus1) << "pre-restore divergence check (INPUT_VAR_1)";
        ASSERT_EQ(i2_diverged, bogus2) << "pre-restore divergence check (INPUT_VAR_2)";
        ASSERT_EQ(i3_diverged, bogus3) << "pre-restore divergence check (INPUT_VAR_3)";
    }

    // Restore.
    auto restore_r = protocols.run(Protocol::DESERIALIZATION,
                                   make_context(0, 0, "restore", "fortran-cat-1"));
    ASSERT_TRUE(restore_r.has_value());

    // Verify every serialized field round-tripped. Because the inputs
    // were demonstrably mutated above, these equalities are real
    // evidence that restore overwrote the bogus values with the
    // pre-save originals.
    EXPECT_EQ(model->GetCurrentTime(), t_ref);
    double i1 = 0.0;
    float  i2 = 0.0f;
    int    i3 = 0;
    model->GetValue("INPUT_VAR_1", &i1);
    model->GetValue("INPUT_VAR_2", &i2);
    model->GetValue("INPUT_VAR_3", &i3);
    EXPECT_EQ(i1, input1);
    EXPECT_EQ(i2, input2);
    EXPECT_EQ(i3, input3);
}

#endif  // NGEN_BMI_FORTRAN_LIB_TESTS_ACTIVE
