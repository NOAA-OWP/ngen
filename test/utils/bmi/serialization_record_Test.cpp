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
Independent tests for the SerializationRecord on-disk format. These tests
exercise the boost::serialization archive round-trip + length-prefix
framing in isolation from any BMI model or protocol so that format
regressions are caught without pulling in the adapter stack.
*/
#include "gtest/gtest.h"

#include "serialization_record.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using models::bmi::protocols::SerializationRecord;
using models::bmi::protocols::write_record;
using models::bmi::protocols::read_next_record;
using models::bmi::protocols::Status;

namespace {
// Local helpers — wire-format reads now return
// expected<Status, string>; bool-style idioms ("while still
// reading") map to has_value() AND value()==Status::Ok. The error
// arm is reserved for malformed records and is checked explicitly
// where the test cares about it.
inline bool ok(const nonstd::expected<Status, std::string>& r) {
    return r.has_value() && r.value() == Status::Ok;
}
inline bool eof(const nonstd::expected<Status, std::string>& r) {
    return r.has_value() && r.value() == Status::Eof;
}
}  // namespace

TEST(SerializationRecordTest, round_trip_single) {
    SerializationRecord out{"cat-1", 7, int64_t{1448982000}, {'a', 'b', 'c', '\0', 'x'}};
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    ASSERT_TRUE(write_record(ss, out).has_value());

    SerializationRecord in;
    ASSERT_TRUE(ok(read_next_record(ss, in)));
    EXPECT_EQ(in.id, out.id);
    EXPECT_EQ(in.time_step, out.time_step);
    EXPECT_EQ(in.simulation_timestamp, out.simulation_timestamp);
    EXPECT_EQ(in.payload, out.payload);
}

TEST(SerializationRecordTest, round_trip_multiple_records) {
    // Simulate multiple save() calls on the same stream — the format must
    // allow reading them back one at a time.
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    std::vector<SerializationRecord> written = {
        {"cat-1", 0, int64_t{0},   {'A'}},
        {"cat-2", 0, int64_t{0},   {'B', 'B'}},
        {"cat-1", 1, int64_t{60},  {'A', 'A', 'A'}},
        {"cat-3", 5, int64_t{1452009600}, std::vector<char>(1024, 0x42)},
    };
    for (const auto& rec : written) ASSERT_TRUE(write_record(ss, rec).has_value());

    std::vector<SerializationRecord> read;
    SerializationRecord scratch;
    while (ok(read_next_record(ss, scratch))) read.push_back(scratch);

    ASSERT_EQ(read.size(), written.size());
    for (size_t i = 0; i < written.size(); ++i) {
        EXPECT_EQ(read[i].id, written[i].id) << "record " << i;
        EXPECT_EQ(read[i].time_step, written[i].time_step) << "record " << i;
        EXPECT_EQ(read[i].simulation_timestamp, written[i].simulation_timestamp) << "record " << i;
        EXPECT_EQ(read[i].payload, written[i].payload) << "record " << i;
    }
}

TEST(SerializationRecordTest, empty_stream_reads_no_records) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    SerializationRecord rec;
    EXPECT_TRUE(eof(read_next_record(ss, rec)));
}

TEST(SerializationRecordTest, empty_payload_round_trip) {
    // A record with zero payload bytes must still be a valid, recoverable record.
    SerializationRecord out{"empty-payload", 42, int64_t{42}, {}};
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    ASSERT_TRUE(write_record(ss, out).has_value());

    SerializationRecord in;
    ASSERT_TRUE(ok(read_next_record(ss, in)));
    EXPECT_EQ(in.id, out.id);
    EXPECT_EQ(in.time_step, out.time_step);
    EXPECT_EQ(in.simulation_timestamp, out.simulation_timestamp);
    EXPECT_TRUE(in.payload.empty());
    EXPECT_TRUE(eof(read_next_record(ss, in)));
}

TEST(SerializationRecordTest, file_backed_append_mode) {
    // The production protocol opens the output stream in append mode on each
    // run(). Reproduce that exact sequence in a file to catch any interaction
    // between fresh archives and ios::app.
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string dir = ::testing::TempDir();
    if (!dir.empty() && dir.back() != '/') dir.push_back('/');
    std::string path = dir + "ngen_serialization_record_" + info->name() + ".bin";
    std::remove(path.c_str());

    std::vector<SerializationRecord> written = {
        {"cat-A", 0, int64_t{0},   {'A', 'A'}},
        {"cat-B", 0, int64_t{0},   {'B'}},
        {"cat-A", 2, int64_t{120}, {'A', 'A', 'A', 'A'}},
    };
    for (const auto& rec : written) {
        std::ofstream ofs(path, std::ios::binary | std::ios::app);
        ASSERT_TRUE(ofs) << "failed to open " << path;
        ASSERT_TRUE(write_record(ofs, rec).has_value());
    }

    std::ifstream ifs(path, std::ios::binary);
    ASSERT_TRUE(ifs);
    std::vector<SerializationRecord> read;
    SerializationRecord scratch;
    while (ok(read_next_record(ifs, scratch))) read.push_back(scratch);

    ASSERT_EQ(read.size(), written.size());
    for (size_t i = 0; i < written.size(); ++i) {
        EXPECT_EQ(read[i].id, written[i].id);
        EXPECT_EQ(read[i].time_step, written[i].time_step);
        EXPECT_EQ(read[i].simulation_timestamp, written[i].simulation_timestamp);
        EXPECT_EQ(read[i].payload, written[i].payload);
    }
    std::remove(path.c_str());
}

// Truncation tolerance: a crashed writer may leave the file ending mid-
// record (inside the prefix, the id bytes, or the payload bytes). All
// three cases must surface as clean Status::Eof so recovery tooling
// can read every intact record up to the truncation point — not as
// the expected<>'s error arm (which is reserved for malformed bytes).
TEST(SerializationRecordTest, truncated_payload_stops_cleanly) {
    namespace wf = models::bmi::protocols::serialization::wire_format;
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    std::vector<SerializationRecord> written = {
        {"cat-1", 0, int64_t{0},  {'A', 'A'}},
        {"cat-2", 1, int64_t{60}, {'B', 'B', 'B'}},
    };
    for (const auto& rec : written) ASSERT_TRUE(write_record(ss, rec).has_value());
    // Manually append a partial third record: a complete prefix
    // claiming a 100-byte payload, but only 4 bytes of payload follow.
    // The reader must stop cleanly at the torn record, returning every
    // intact record before it.
    wf::RecordPrefix prefix;
    prefix.id_length      = 4;
    prefix.payload_length = 100;
    wf::write_record_prefix(ss, prefix);
    const char id_bytes[] = {'c', 'a', 't', '3'};
    ss.write(id_bytes, sizeof(id_bytes));
    const char partial_payload[] = {'X', 'X', 'X', 'X'};
    ss.write(partial_payload, sizeof(partial_payload));

    std::vector<SerializationRecord> read;
    SerializationRecord scratch;
    while (ok(read_next_record(ss, scratch))) read.push_back(scratch);

    ASSERT_EQ(read.size(), 2u);
    EXPECT_EQ(read[0].id, "cat-1");
    EXPECT_EQ(read[1].id, "cat-2");
}

// Truncated inside the fixed-size prefix itself — the reader must not
// mistake a partial prefix for any successfully-read record.
TEST(SerializationRecordTest, truncated_prefix_stops_cleanly) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    SerializationRecord rec{"cat-1", 0, int64_t{0}, {'A'}};
    ASSERT_TRUE(write_record(ss, rec).has_value());
    // Append 4 bytes — far fewer than a complete prefix needs. The
    // reader sees a short read inside the prefix and returns false.
    const uint32_t half = 0xDEADBEEF;
    ss.write(reinterpret_cast<const char*>(&half), sizeof(half));

    std::vector<SerializationRecord> read;
    SerializationRecord scratch;
    while (ok(read_next_record(ss, scratch))) read.push_back(scratch);
    ASSERT_EQ(read.size(), 1u);
    EXPECT_EQ(read[0].id, "cat-1");
}

// Magic-byte mismatch — bytes that look like a complete prefix but
// don't start with 'NGSR' are corruption, not truncation. The reader
// returns the error arm with a diagnostic about the magic word
// rather than treating the bytes as a record with garbage values.
TEST(SerializationRecordTest, magic_mismatch_returns_error) {
    namespace wf = models::bmi::protocols::serialization::wire_format;
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    // Write a full prefix's worth of bytes with a corrupted magic
    // word. Subsequent fields are valid-looking but never reached —
    // the reader rejects on the magic check first.
    wf::RecordPrefix prefix;
    prefix.magic        = 0xDEADBEEFu;  // not 'NGSR'
    prefix.id_length    = 0;
    prefix.payload_length = 0;
    wf::write_record_prefix(ss, prefix);

    SerializationRecord rec;
    auto r = read_next_record(ss, rec);
    ASSERT_FALSE(r.has_value());
    EXPECT_NE(r.error().find("bad magic"), std::string::npos)
        << "error message was: " << r.error();
}

// Oversized payload_length — a byte-flipped or tampered prefix in an
// otherwise-valid-looking file is rejected via the error arm rather
// than silently driving a multi-gigabyte allocation. This is what
// keeps `read_next_record` safe against corrupted inputs across all
// callers (the index builder, external tooling, etc).
TEST(SerializationRecordTest, oversized_payload_length_returns_error) {
    namespace wf = models::bmi::protocols::serialization::wire_format;
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    // Write a fully-valid prefix EXCEPT payload_length exceeds the cap.
    wf::RecordPrefix prefix;
    prefix.id_length      = 0;
    prefix.payload_length = models::bmi::protocols::MAX_RECORD_PAYLOAD_BYTES + 1;
    wf::write_record_prefix(ss, prefix);

    SerializationRecord rec;
    auto r = read_next_record(ss, rec);
    ASSERT_FALSE(r.has_value());
    EXPECT_NE(r.error().find("MAX_RECORD_PAYLOAD_BYTES"), std::string::npos)
        << "error message was: " << r.error();
}
