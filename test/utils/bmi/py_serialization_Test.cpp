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
End-to-end BMI serialization protocol test against a Python model.

Exercises the full save/restore lifecycle through Bmi_Py_Adapter to
validate three parallel contracts:

  1. The Python adapter's byte-typed dispatch (uint8) marshals opaque
     payloads across the C++/Python boundary in both directions.
  2. The restore-side SetValue(serialization_size, ...) correctly
     pre-sizes the Python model's receive buffer so the subsequent
     SetValue(serialization_state, ...) carries all the bytes.
  3. The Python test model's pickle-based serialize/deserialize
     helpers round-trip the model's scalar state faithfully.

The C/C++/Fortran sibling tests live in serialization_Test.cpp and
deserialization_Test.cpp; this file is the Python analog.
*/
#ifdef NGEN_BMI_PY_TESTS_ACTIVE

#include "gtest/gtest.h"
#include "MockConfig.hpp"
#include "FileChecker.h"

#include <pybind11/embed.h>
namespace py = pybind11;

#include "Bmi_Py_Adapter.hpp"
#include "protocols.hpp"
#include "serialization_record.hpp"
#include "utilities/python/InterpreterUtil.hpp"

#include <cstdio>
#include <fstream>
#include <memory>
#include <sys/stat.h>

using models::bmi::Bmi_Py_Adapter;
using models::bmi::protocols::NgenBmiProtocols;
using models::bmi::protocols::Protocol;
using models::bmi::protocols::ProtocolError;
using models::bmi::protocols::SerializationRecord;
using models::bmi::protocols::read_next_record;
using utils::ngenPy::InterpreterUtil;

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
}

class Bmi_Py_Serialization_Test : public ::testing::Test {
protected:
    // Keep the embedded interpreter alive for the whole test suite —
    // InterpreterUtil returns a refcounted handle; the first test that
    // constructs one starts the interpreter, subsequent tests share it.
    static std::shared_ptr<InterpreterUtil> interpreter;

    static void SetUpTestSuite() {
        // Point Python at the bundled test_bmi_py package.
        InterpreterUtil::addToPyPath("./extern/");
    }

    void SetUp() override {
        model_name = "test_bmi_py.bmi_model";
        bmi_init_config = "./test/data/bmi/test_bmi_python/test_bmi_python_config_0.yml";
        model = std::make_shared<Bmi_Py_Adapter>(
            model_name, bmi_init_config, model_name, /*allow_exceed_end_time*/ true);

        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        std::string dir = ::testing::TempDir();
        if (!dir.empty() && dir.back() != '/') dir.push_back('/');
        path = dir + "ngen_py_serialization_Test_" + info->name() + ".ckpt";
        std::remove(path.c_str());
    }

    void TearDown() override {
        std::remove(path.c_str());
    }

    std::string model_name;
    std::string bmi_init_config;
    std::shared_ptr<Bmi_Py_Adapter> model;
    std::string path;
};

// The interpreter lifetime spans every test in the suite.
std::shared_ptr<InterpreterUtil> Bmi_Py_Serialization_Test::interpreter = InterpreterUtil::getInstance();

// ---------------------------------------------------------------------
// Support detection passes against a conforming Python model.
// ---------------------------------------------------------------------

TEST_F(Bmi_Py_Serialization_Test, check_support_passes) {
    // A fully-configured save block with check=true will run
    // check_support at construction time. Any units-mismatch or
    // missing reserved variable would escalate to a PROTOCOL_ERROR
    // under the default fatal=true — a clean construction implies
    // check_support passed.
    auto properties = SerializationMock(path).as_json_property();
    EXPECT_NO_THROW({
        auto protocols = NgenBmiProtocols(
            std::static_pointer_cast<models::bmi::Bmi_Adapter>(model),
            properties);
        (void)protocols;
    });
}

// ---------------------------------------------------------------------
// Save side: one run() call writes one record with the configured id.
// ---------------------------------------------------------------------

TEST_F(Bmi_Py_Serialization_Test, save_writes_record) {
    auto properties = SerializationMock(path).as_json_property();
    auto protocols = NgenBmiProtocols(
        std::static_pointer_cast<models::bmi::Bmi_Adapter>(model), properties);

    auto result = protocols.run(Protocol::SERIALIZATION,
                                make_context(0, 2, "0", "py-cat-1"));
    EXPECT_TRUE(result.has_value());

    // Byte-exact validation: rather than checking a weakly-true
    // invariant like "payload is non-empty", reproduce the model's
    // capture sequence manually on the (unchanged) model after the
    // protocol's save and confirm the record carries those same
    // bytes. The protocol's FREE already ran, so the model is back
    // to size=0 / state=empty — running CREATE again against the
    // same underlying state produces a byte-identical pickle blob
    // because CPython dict ordering is insertion-stable and numpy
    // / float pickling is deterministic. Any adapter-layer byte
    // corruption or truncation between `GetValue(state, ...)` and
    // the record payload would light up here.
    int trigger = 1;
    int expected_size = 0;
    model->SetValue("ngen::serialization_create", &trigger);
    model->GetValue("ngen::serialization_size", &expected_size);
    ASSERT_GT(expected_size, 0);
    std::vector<char> expected_payload(static_cast<size_t>(expected_size));
    model->GetValue("ngen::serialization_state", expected_payload.data());
    model->SetValue("ngen::serialization_free", &trigger);

    auto records = read_all_records(path);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].id, "py-cat-1");
    EXPECT_EQ(records[0].time_step, 0);
    EXPECT_EQ(records[0].payload.size(), expected_payload.size());
    EXPECT_EQ(records[0].payload, expected_payload);
}

// ---------------------------------------------------------------------
// Integration: save -> restore round trip through one configuration.
// Demonstrates the full lifecycle of the Python byte-typed adapter
// path and the restore-side SetValue(size, ...) pre-sizing step.
// ---------------------------------------------------------------------

TEST_F(Bmi_Py_Serialization_Test, save_restore_roundtrip) {
    // One config drives both directions; save fires every step,
    // restore picks the latest record found for this id.
    auto properties = DeserializationMock(path, /*step*/ "latest")
        .with_save(/*frequency*/ 1)
        .as_json_property();
    auto protocols = NgenBmiProtocols(
        std::static_pointer_cast<models::bmi::Bmi_Adapter>(model), properties);

    // Prime the model with distinct input scalars so the pickle bytes
    // carry meaningful content we can later verify.
    double input1 = 11.5;
    double input2 = 22.25;
    model->SetValue("INPUT_VAR_1", &input1);
    model->SetValue("INPUT_VAR_2", &input2);
    model->Update();

    const double t_ref = model->GetCurrentTime();

    // Save at step 1.
    auto save_r = protocols.run(Protocol::SERIALIZATION,
                                make_context(1, 3, "3600", "py-cat-1"));
    ASSERT_TRUE(save_r.has_value());
    ASSERT_TRUE(file_exists(path));

    // Diverge the model's state explicitly — overwrite the serialized
    // inputs with clearly-different values AND advance the clock. A
    // plain Update() would only move `current_model_time`, leaving the
    // inputs untouched, which means a silently-broken restore could
    // pass the post-restore input checks simply because the inputs
    // were never actually perturbed. Sanity-checking the divergence
    // before restore ensures the round-trip assertion below is
    // measuring something real.
    const double bogus1 = -777.0;
    const double bogus2 = -888.0;
    double bogus1_copy = bogus1;
    double bogus2_copy = bogus2;
    model->SetValue("INPUT_VAR_1", &bogus1_copy);
    model->SetValue("INPUT_VAR_2", &bogus2_copy);
    model->Update();
    model->Update();
    ASSERT_NE(model->GetCurrentTime(), t_ref);

    {
        double i1_diverged = 0.0;
        double i2_diverged = 0.0;
        model->GetValue("INPUT_VAR_1", &i1_diverged);
        model->GetValue("INPUT_VAR_2", &i2_diverged);
        ASSERT_EQ(i1_diverged, bogus1) << "pre-restore divergence check";
        ASSERT_EQ(i2_diverged, bogus2) << "pre-restore divergence check";
    }

    // Restore.
    auto restore_r = protocols.run(Protocol::DESERIALIZATION,
                                   make_context(0, 0, "restore", "py-cat-1"));
    ASSERT_TRUE(restore_r.has_value());

    // Verify every serialized field round-tripped. Because the inputs
    // were demonstrably mutated above, these equalities are real
    // evidence that restore overwrote the bogus values with the
    // pre-save originals.
    EXPECT_EQ(model->GetCurrentTime(), t_ref);
    double i1 = 0.0, i2 = 0.0;
    model->GetValue("INPUT_VAR_1", &i1);
    model->GetValue("INPUT_VAR_2", &i2);
    EXPECT_EQ(i1, input1);
    EXPECT_EQ(i2, input2);
}

#endif  // NGEN_BMI_PY_TESTS_ACTIVE
