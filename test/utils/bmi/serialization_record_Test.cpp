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

TEST(SerializationRecordTest, round_trip_single) {
    SerializationRecord out{"cat-1", 7, int64_t{1448982000}, {'a', 'b', 'c', '\0', 'x'}};
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    write_record(ss, out);

    SerializationRecord in;
    ASSERT_TRUE(read_next_record(ss, in));
    // Unary `+` promotes the uint8_t to int for gtest — avoids ODR-use of the
    // static constexpr in C++14 and prints the value numerically on failure.
    EXPECT_EQ(+in.version, +SerializationRecord::CURRENT_VERSION);
    EXPECT_EQ(in.id, out.id);
    EXPECT_EQ(in.time_step, out.time_step);
    EXPECT_EQ(in.timestamp, out.timestamp);
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
    for (const auto& rec : written) write_record(ss, rec);

    std::vector<SerializationRecord> read;
    SerializationRecord scratch;
    while (read_next_record(ss, scratch)) read.push_back(scratch);

    ASSERT_EQ(read.size(), written.size());
    for (size_t i = 0; i < written.size(); ++i) {
        EXPECT_EQ(read[i].id, written[i].id) << "record " << i;
        EXPECT_EQ(read[i].time_step, written[i].time_step) << "record " << i;
        EXPECT_EQ(read[i].timestamp, written[i].timestamp) << "record " << i;
        EXPECT_EQ(read[i].payload, written[i].payload) << "record " << i;
    }
}

TEST(SerializationRecordTest, empty_stream_reads_no_records) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    SerializationRecord rec;
    EXPECT_FALSE(read_next_record(ss, rec));
}

TEST(SerializationRecordTest, empty_payload_round_trip) {
    // A record with zero payload bytes must still be a valid, recoverable record.
    SerializationRecord out{"empty-payload", 42, int64_t{42}, {}};
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    write_record(ss, out);

    SerializationRecord in;
    ASSERT_TRUE(read_next_record(ss, in));
    EXPECT_EQ(in.id, out.id);
    EXPECT_EQ(in.time_step, out.time_step);
    EXPECT_EQ(in.timestamp, out.timestamp);
    EXPECT_TRUE(in.payload.empty());
    EXPECT_FALSE(read_next_record(ss, in));
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
        write_record(ofs, rec);
    }

    std::ifstream ifs(path, std::ios::binary);
    ASSERT_TRUE(ifs);
    std::vector<SerializationRecord> read;
    SerializationRecord scratch;
    while (read_next_record(ifs, scratch)) read.push_back(scratch);

    ASSERT_EQ(read.size(), written.size());
    for (size_t i = 0; i < written.size(); ++i) {
        EXPECT_EQ(read[i].id, written[i].id);
        EXPECT_EQ(read[i].time_step, written[i].time_step);
        EXPECT_EQ(read[i].timestamp, written[i].timestamp);
        EXPECT_EQ(read[i].payload, written[i].payload);
    }
    std::remove(path.c_str());
}

// Truncation tolerance: a crashed writer may leave the file ending mid-
// record (either inside the length prefix or inside the archive bytes).
// Both cases must surface as clean end-of-stream so recovery tooling can
// read every intact record up to the truncation point without a throw.
TEST(SerializationRecordTest, truncated_file_stops_cleanly) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    std::vector<SerializationRecord> written = {
        {"cat-1", 0, int64_t{0},  {'A', 'A'}},
        {"cat-2", 1, int64_t{60}, {'B', 'B', 'B'}},
    };
    for (const auto& rec : written) write_record(ss, rec);
    // Manually write a partial third record: 8-byte length prefix + a
    // truncated body (only 4 bytes where length claims 10000).
    const uint64_t bogus_length = 10000;
    ss.write(reinterpret_cast<const char*>(&bogus_length), sizeof(bogus_length));
    const char partial[] = {'X', 'X', 'X', 'X'};
    ss.write(partial, sizeof(partial));

    std::vector<SerializationRecord> read;
    SerializationRecord scratch;
    while (read_next_record(ss, scratch)) read.push_back(scratch);

    // Only the two complete records should have come back; the torn tail
    // returns false without throwing.
    ASSERT_EQ(read.size(), 2u);
    EXPECT_EQ(read[0].id, "cat-1");
    EXPECT_EQ(read[1].id, "cat-2");
}

// Truncated inside the 8-byte length prefix itself — the reader must not
// confuse a partial length header for a zero-length record.
TEST(SerializationRecordTest, truncated_header_stops_cleanly) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    SerializationRecord rec{"cat-1", 0, int64_t{0}, {'A'}};
    write_record(ss, rec);
    // Write 4 bytes of a "next" length prefix — not a complete uint64_t.
    const uint32_t half = 0xDEADBEEF;
    ss.write(reinterpret_cast<const char*>(&half), sizeof(half));

    std::vector<SerializationRecord> read;
    SerializationRecord scratch;
    while (read_next_record(ss, scratch)) read.push_back(scratch);
    ASSERT_EQ(read.size(), 1u);
    EXPECT_EQ(read[0].id, "cat-1");
}

// Oversized length prefix — a byte-flipped or tampered length in an
// otherwise-valid-looking file is rejected with an exception rather
// than silently driving a multi-gigabyte allocation. This is what
// keeps `read_next_record` safe against corrupted inputs across
// all callers (the index builder, external tooling, etc).
TEST(SerializationRecordTest, oversized_length_throws) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    // Write a length prefix that claims the archive body is larger
    // than MAX_RECORD_ARCHIVE_BYTES (1 GiB). No bytes follow.
    const uint64_t huge = models::bmi::protocols::MAX_RECORD_ARCHIVE_BYTES + 1;
    ss.write(reinterpret_cast<const char*>(&huge), sizeof(huge));

    SerializationRecord rec;
    EXPECT_THROW(read_next_record(ss, rec), std::runtime_error);
}
