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
Tests for the portable little-endian byte I/O primitives in byte_io.hpp.
These exercise each integer-width helper in isolation — round-trip,
explicit byte layout, short-read safety, and the two's-complement
canaries that validate the signed helpers on the running host.

Format-level tests for RecordPrefix (the consumer of these primitives)
live in wire_format_Test.cpp.
*/
#include "gtest/gtest.h"

#include "byte_io.hpp"

#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

namespace bio = models::bmi::protocols::serialization::byte_io;

// ---- Round-trip coverage --------------------------------------------------

TEST(ByteIO, u16_round_trip) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bio::write_u16_le(ss, 0x0000);
    bio::write_u16_le(ss, 0x00FF);
    bio::write_u16_le(ss, 0xFF00);
    bio::write_u16_le(ss, 0xFFFF);
    bio::write_u16_le(ss, 0x1234);

    uint16_t v = 0;
    ASSERT_TRUE(bio::read_u16_le(ss, v));
    EXPECT_EQ(v, 0x0000);
    ASSERT_TRUE(bio::read_u16_le(ss, v));
    EXPECT_EQ(v, 0x00FF);
    ASSERT_TRUE(bio::read_u16_le(ss, v));
    EXPECT_EQ(v, 0xFF00);
    ASSERT_TRUE(bio::read_u16_le(ss, v));
    EXPECT_EQ(v, 0xFFFF);
    ASSERT_TRUE(bio::read_u16_le(ss, v));
    EXPECT_EQ(v, 0x1234);
}

TEST(ByteIO, u32_round_trip) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bio::write_u32_le(ss, 0u);
    bio::write_u32_le(ss, std::numeric_limits<uint32_t>::max());
    bio::write_u32_le(ss, 0xDEADBEEFu);

    uint32_t v = 0;
    ASSERT_TRUE(bio::read_u32_le(ss, v));
    EXPECT_EQ(v, 0u);
    ASSERT_TRUE(bio::read_u32_le(ss, v));
    EXPECT_EQ(v, std::numeric_limits<uint32_t>::max());
    ASSERT_TRUE(bio::read_u32_le(ss, v));
    EXPECT_EQ(v, 0xDEADBEEFu);
}

TEST(ByteIO, u64_round_trip) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bio::write_u64_le(ss, 0ull);
    bio::write_u64_le(ss, std::numeric_limits<uint64_t>::max());
    bio::write_u64_le(ss, 0x0123456789ABCDEFull);

    uint64_t v = 0;
    ASSERT_TRUE(bio::read_u64_le(ss, v));
    EXPECT_EQ(v, 0ull);
    ASSERT_TRUE(bio::read_u64_le(ss, v));
    EXPECT_EQ(v, std::numeric_limits<uint64_t>::max());
    ASSERT_TRUE(bio::read_u64_le(ss, v));
    EXPECT_EQ(v, 0x0123456789ABCDEFull);
}

TEST(ByteIO, i32_round_trip_signed) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bio::write_i32_le(ss, 0);
    bio::write_i32_le(ss, std::numeric_limits<int32_t>::min());
    bio::write_i32_le(ss, std::numeric_limits<int32_t>::max());
    bio::write_i32_le(ss, -1);
    bio::write_i32_le(ss, -1234567);

    int32_t v = 0;
    ASSERT_TRUE(bio::read_i32_le(ss, v));
    EXPECT_EQ(v, 0);
    ASSERT_TRUE(bio::read_i32_le(ss, v));
    EXPECT_EQ(v, std::numeric_limits<int32_t>::min());
    ASSERT_TRUE(bio::read_i32_le(ss, v));
    EXPECT_EQ(v, std::numeric_limits<int32_t>::max());
    ASSERT_TRUE(bio::read_i32_le(ss, v));
    EXPECT_EQ(v, -1);
    ASSERT_TRUE(bio::read_i32_le(ss, v));
    EXPECT_EQ(v, -1234567);
}

TEST(ByteIO, i64_round_trip_signed) {
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bio::write_i64_le(ss, 0);
    bio::write_i64_le(ss, std::numeric_limits<int64_t>::min());
    bio::write_i64_le(ss, std::numeric_limits<int64_t>::max());
    bio::write_i64_le(ss, int64_t{-1});
    bio::write_i64_le(ss, int64_t{1448982000}); // a plausible epoch timestamp

    int64_t v = 0;
    ASSERT_TRUE(bio::read_i64_le(ss, v));
    EXPECT_EQ(v, 0);
    ASSERT_TRUE(bio::read_i64_le(ss, v));
    EXPECT_EQ(v, std::numeric_limits<int64_t>::min());
    ASSERT_TRUE(bio::read_i64_le(ss, v));
    EXPECT_EQ(v, std::numeric_limits<int64_t>::max());
    ASSERT_TRUE(bio::read_i64_le(ss, v));
    EXPECT_EQ(v, int64_t{-1});
    ASSERT_TRUE(bio::read_i64_le(ss, v));
    EXPECT_EQ(v, int64_t{1448982000});
}

// ---- Explicit byte layout -------------------------------------------------

TEST(ByteIO, u32_explicit_byte_layout) {
    // 0x12345678 must hit disk as bytes [0x78, 0x56, 0x34, 0x12] —
    // the little-endian convention this module promises.
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bio::write_u32_le(ss, 0x12345678u);
    const std::string bytes = ss.str();
    ASSERT_EQ(bytes.size(), 4u);
    EXPECT_EQ(static_cast<uint8_t>(bytes[0]), 0x78u);
    EXPECT_EQ(static_cast<uint8_t>(bytes[1]), 0x56u);
    EXPECT_EQ(static_cast<uint8_t>(bytes[2]), 0x34u);
    EXPECT_EQ(static_cast<uint8_t>(bytes[3]), 0x12u);
}

// ---- Two's-complement canary tests ----------------------------------------
//
// These two tests are the platform-validity canaries the wire format
// relies on. They verify that `int{32,64}_t{-1}` writes as all-0xFF
// bytes on disk, which is true if and only if the running host uses
// two's-complement signed integer representation. A ones'-complement
// host would produce 0xFE in the low byte (signed -1 is 0xFE in 1C,
// not 0xFF); a sign-magnitude host would produce 0x01 plus the sign-
// bit byte 0x80 — both layouts these tests reject.
//
// **If these tests fail on a new build target, the wire format's
// signed integer fields cannot be safely written or read on that
// target.** See wire_format.hpp's "Signed integer fields and the
// two's-complement assumption" section for the full explanation of
// what these canaries validate at the format level.

TEST(ByteIO, i32_explicit_byte_layout_negative) {
    // -1 (int32) is 0xFFFFFFFF in two's complement; on disk every byte
    // should be 0xFF. See "Two's-complement canary tests" comment above
    // for the full implication if this test fails.
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bio::write_i32_le(ss, int32_t{-1});
    const std::string bytes = ss.str();
    ASSERT_EQ(bytes.size(), 4u);
    for (char b : bytes) EXPECT_EQ(static_cast<uint8_t>(b), 0xFFu);
}

TEST(ByteIO, i64_explicit_byte_layout_negative) {
    // -1 (int64) is 0xFFFFFFFFFFFFFFFF in two's complement; on disk every
    // byte should be 0xFF. The int32 round-trip test alone cannot catch
    // a non-two's-complement int64 — they're separate types and the
    // compiler's representation choices could in principle differ —
    // so this canary exists as the int64 counterpart.
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    bio::write_i64_le(ss, int64_t{-1});
    const std::string bytes = ss.str();
    ASSERT_EQ(bytes.size(), 8u);
    for (char b : bytes) EXPECT_EQ(static_cast<uint8_t>(b), 0xFFu);
}

// ---- Short-read safety ----------------------------------------------------

TEST(ByteIO, short_read_returns_false) {
    // Two bytes written; u32 read needs four — must return false
    // without throwing or reading garbage.
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    const char partial[2] = {0x01, 0x02};
    ss.write(partial, 2);
    uint32_t v = 0xCAFEu;
    EXPECT_FALSE(bio::read_u32_le(ss, v));
}
