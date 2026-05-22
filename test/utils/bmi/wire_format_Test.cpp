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
Tests for RecordPrefix and its read/write helpers in wire_format.hpp.
These exercise the prefix as a whole — round trip, exact byte layout,
magic-byte identification, truncation tolerance, and field-width
boundary conditions.

Tests for the underlying field-level byte I/O primitives (write_u32_le,
read_i64_le, the two's-complement canaries, etc.) live in
byte_io_Test.cpp.
*/
#include "gtest/gtest.h"

#include "wire_format.hpp"

#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

namespace wf = models::bmi::protocols::serialization::wire_format;

TEST(WireFormatPrefix, round_trip) {
    wf::RecordPrefix out;
    out.time_step            = 42;
    out.simulation_timestamp = 1448982000;   // 2015-12-01T15:00:00 UTC
    out.checkpoint_epoch     = 1779580800;   // 2026-05-22T15:20:00 UTC
    out.id_length            = 5;
    out.payload_length       = 1024;

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    wf::write_record_prefix(ss, out);
    EXPECT_EQ(ss.str().size(), wf::RecordPrefix::PREFIX_BYTES);

    wf::RecordPrefix in;
    ASSERT_TRUE(wf::read_record_prefix(ss, in));
    EXPECT_EQ(in.magic,                out.magic);
    EXPECT_EQ(in.wire_version,         out.wire_version);
    EXPECT_EQ(in.time_step,            out.time_step);
    EXPECT_EQ(in.simulation_timestamp, out.simulation_timestamp);
    EXPECT_EQ(in.checkpoint_epoch,     out.checkpoint_epoch);
    EXPECT_EQ(in.id_length,            out.id_length);
    EXPECT_EQ(in.payload_length,       out.payload_length);
}

TEST(WireFormatPrefix, timestamp_fields_handle_pre_unix_epoch) {
    // Both timestamp fields are signed seconds since the Unix epoch.
    // Negative values must round-trip cleanly to support paleoclimate
    // simulations (modeling 1850, 0001 BC, etc.) and any other
    // pre-1970 reference time.
    wf::RecordPrefix out;
    out.simulation_timestamp = -3786825600;  // 1850-01-01T00:00:00 UTC
    out.checkpoint_epoch     = 1779580800;

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    wf::write_record_prefix(ss, out);

    wf::RecordPrefix in;
    ASSERT_TRUE(wf::read_record_prefix(ss, in));
    EXPECT_EQ(in.simulation_timestamp, int64_t{-3786825600});
    EXPECT_EQ(in.checkpoint_epoch,     int64_t{1779580800});
}

TEST(WireFormatPrefix, prefix_bytes_constant_matches_field_sum) {
    // The PREFIX_BYTES constant is what skip-past-prefix code uses;
    // verify it equals the sum of the underlying field widths
    // (magic, wire_version, time_step, simulation_timestamp,
    //  checkpoint_epoch, id_length, payload_length).
    EXPECT_EQ(wf::RecordPrefix::PREFIX_BYTES,
              sizeof(uint32_t) + sizeof(uint8_t)  +
              sizeof(int64_t)  + sizeof(int64_t)  + sizeof(int64_t) +
              sizeof(uint16_t) + sizeof(uint64_t));
    EXPECT_EQ(wf::RecordPrefix::PREFIX_BYTES, 39u);
}

TEST(WireFormatPrefix, magic_disk_bytes_spell_NGSR) {
    // The on-disk leading bytes of every record file must be the
    // ASCII text "NGSR" — usable as a file-type identifier outside
    // of this code (file(1), hex dumps, etc.).
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    wf::RecordPrefix p;  // defaults: magic = MAGIC
    wf::write_record_prefix(ss, p);
    const std::string bytes = ss.str();
    ASSERT_GE(bytes.size(), 4u);
    EXPECT_EQ(bytes[0], 'N');
    EXPECT_EQ(bytes[1], 'G');
    EXPECT_EQ(bytes[2], 'S');
    EXPECT_EQ(bytes[3], 'R');
}

TEST(WireFormatPrefix, empty_stream_returns_false) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    wf::RecordPrefix p;
    EXPECT_FALSE(wf::read_record_prefix(ss, p));
}

TEST(WireFormatPrefix, truncated_prefix_returns_false) {
    // Write a full prefix then truncate the bytes mid-prefix at every
    // boundary. Each truncation must surface as false (clean EOF) rather
    // than as a partially-populated prefix or a throw.
    wf::RecordPrefix out;
    out.time_step            = 7;
    out.simulation_timestamp = 1448982000;
    out.checkpoint_epoch     = 1779580800;
    out.id_length            = 3;
    out.payload_length       = 99;

    std::stringstream full(std::ios::in | std::ios::out | std::ios::binary);
    wf::write_record_prefix(full, out);
    const std::string complete = full.str();
    ASSERT_EQ(complete.size(), wf::RecordPrefix::PREFIX_BYTES);

    for (std::size_t cut = 0; cut < complete.size(); ++cut) {
        std::stringstream ss(std::string(complete.data(), cut),
                             std::ios::in | std::ios::binary);
        wf::RecordPrefix in;
        EXPECT_FALSE(wf::read_record_prefix(ss, in))
            << "truncation at " << cut << " unexpectedly succeeded";
    }
}

TEST(WireFormatPrefix, max_id_length_round_trip) {
    wf::RecordPrefix out;
    out.id_length = std::numeric_limits<uint16_t>::max();
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    wf::write_record_prefix(ss, out);

    wf::RecordPrefix in;
    ASSERT_TRUE(wf::read_record_prefix(ss, in));
    EXPECT_EQ(in.id_length, std::numeric_limits<uint16_t>::max());
}

TEST(WireFormatPrefix, max_payload_length_field_round_trip) {
    // Verifies the payload_length *integer* field round-trips at its
    // maximum value. The actual byte cap on stored payloads is a
    // policy concern of the record layer (MAX_RECORD_*), not of the
    // wire format helpers tested here.
    //
    // payload_length is uint64_t (8 bytes on disk), not uint32_t —
    // narrowing it would have silently capped records at 4 GiB where
    // the v0.1 length prefix had no practical cap.
    wf::RecordPrefix out;
    out.payload_length = std::numeric_limits<uint64_t>::max();
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    wf::write_record_prefix(ss, out);

    wf::RecordPrefix in;
    ASSERT_TRUE(wf::read_record_prefix(ss, in));
    EXPECT_EQ(in.payload_length, std::numeric_limits<uint64_t>::max());
}
