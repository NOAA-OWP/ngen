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
INTERNAL — implementation detail of wire_format::RecordPrefix.

Names the byte-level primitives `wire_format` uses, in one place:

  * Buffer-level: `store_little_uXX` / `load_little_uXX` (and the
    signed `sXX` counterparts) act on `uint8_t*` buffers and compute 
    values arithmetically from byte sequences.
    This ensures the wire-format's cross-host portability: same bytes on disk from any
    host, correct numeric values when read on any host.

  * `store_little_u8` / `load_little_u8` are single-byte trivials defined here.
    These are just for consistency.

  * Stream adapters (`write_uXX_le(ostream, v)`,
    `read_uXX_le(istream, v_out)`) wrap the buffer primitives for
    callers that work with `std::ostream` / `std::istream`.
    These encapsulate the stack-buffer-then-cast-then-write pattern
    so the `reinterpret_cast<char*>(uint8_t*)` bridge between the
    primitives' byte type and the iostream `char*` API is in one
    place.

Signed-integer encoding inherits boost::endian's two's-complement
assumption ([1], [2]); `wire_format.hpp`'s run-host canary tests
verify the host satisfies it. C++20 codifies two's-complement as a
mandatory representation, so once the project bumps to C++20 the
canary tests become formalities.

  [1] versioned (matches the Boost we link against):
      https://www.boost.org/doc/libs/1_86_0/libs/endian/doc/html/endian.html
  [2] latest release (in case the versioned page diverges):
      https://www.boost.org/doc/libs/release/libs/endian/doc/html/endian.html

Visibility is enforced by include-path discipline: this header is
only on the PRIVATE include path of the `ngen_bmi_protocols` library's
own translation units and of the `test_bmi_protocols` test target. It
is never exported via `target_include_directories(PUBLIC ...)`.
*/
#pragma once

#include <boost/endian/conversion.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <type_traits>

namespace models {
namespace bmi {
namespace protocols {
namespace serialization {
namespace byte_io {

// Compile-time guard for the `reinterpret_cast<char*>(uint8_t*)`
// bridge in the stream adapters below. The standard does not
// mandate that `std::uint8_t` alias `unsigned char` — it only
// requires that the type exist, be unsigned, and be exactly 8
// bits. In most scenarios the two are aliases, and the
// reinterpret_cast is well-defined via the standard's
// `char`/`unsigned char` aliasing rules. The assertion below
// catches any future target where the alias does not hold.
static_assert(
    std::is_same<std::uint8_t, unsigned char>::value,
    "byte_io: std::uint8_t must alias unsigned char on this "
    "target — the stream adapters' reinterpret_cast<char*> "
    "bridge depends on it. See the comment immediately above "
    "this assertion for the rationale."
);

// ============================================================
// Buffer encode/decode primitives needed from boost::endian
// Using the convience wrappers around their template load/store.
// ============================================================

using boost::endian::load_little_s32;
using boost::endian::load_little_s64;
using boost::endian::load_little_u16;
using boost::endian::load_little_u32;
using boost::endian::load_little_u64;
using boost::endian::store_little_s32;
using boost::endian::store_little_s64;
using boost::endian::store_little_u16;
using boost::endian::store_little_u32;
using boost::endian::store_little_u64;

// Added for consistency, these are essentially noops
inline void store_little_u8(uint8_t* p, uint8_t v) {
    p[0] = v;
}

inline uint8_t load_little_u8(const uint8_t* p) {
    return p[0];
}

// ============================================================
// Stream adapters — wrap the buffer primitives for callers that
// work with iostream rather than raw byte buffers.
//
// Each adapter contains the `reinterpret_cast<char*>(uint8_t*)`
// that bridges the buffer primitives' byte type to the iostream
// API. The cast is guarded by the static_assert at the top of
// this header.
// ============================================================

inline void write_u8(std::ostream& out, uint8_t v) {
    uint8_t buf[1];
    store_little_u8(buf, v);
    out.write(reinterpret_cast<const char*>(buf), 1);
}

inline bool read_u8(std::istream& in, uint8_t& v) {
    uint8_t buf[1];
    if (!in.read(reinterpret_cast<char*>(buf), 1)) return false;
    if (in.gcount() != 1) return false;
    v = load_little_u8(buf);
    return true;
}

inline void write_u16_le(std::ostream& out, uint16_t v) {
    uint8_t buf[2];
    store_little_u16(buf, v);
    out.write(reinterpret_cast<const char*>(buf), 2);
}

inline bool read_u16_le(std::istream& in, uint16_t& v) {
    uint8_t buf[2];
    if (!in.read(reinterpret_cast<char*>(buf), 2)) return false;
    if (in.gcount() != 2) return false;
    v = load_little_u16(buf);
    return true;
}

inline void write_u32_le(std::ostream& out, uint32_t v) {
    uint8_t buf[4];
    store_little_u32(buf, v);
    out.write(reinterpret_cast<const char*>(buf), 4);
}

inline bool read_u32_le(std::istream& in, uint32_t& v) {
    uint8_t buf[4];
    if (!in.read(reinterpret_cast<char*>(buf), 4)) return false;
    if (in.gcount() != 4) return false;
    v = load_little_u32(buf);
    return true;
}

inline void write_u64_le(std::ostream& out, uint64_t v) {
    uint8_t buf[8];
    store_little_u64(buf, v);
    out.write(reinterpret_cast<const char*>(buf), 8);
}

inline bool read_u64_le(std::istream& in, uint64_t& v) {
    uint8_t buf[8];
    if (!in.read(reinterpret_cast<char*>(buf), 8)) return false;
    if (in.gcount() != 8) return false;
    v = load_little_u64(buf);
    return true;
}

inline void write_i32_le(std::ostream& out, int32_t v) {
    uint8_t buf[4];
    store_little_s32(buf, v);
    out.write(reinterpret_cast<const char*>(buf), 4);
}

inline bool read_i32_le(std::istream& in, int32_t& v) {
    uint8_t buf[4];
    if (!in.read(reinterpret_cast<char*>(buf), 4)) return false;
    if (in.gcount() != 4) return false;
    v = load_little_s32(buf);
    return true;
}

inline void write_i64_le(std::ostream& out, int64_t v) {
    uint8_t buf[8];
    store_little_s64(buf, v);
    out.write(reinterpret_cast<const char*>(buf), 8);
}

inline bool read_i64_le(std::istream& in, int64_t& v) {
    uint8_t buf[8];
    if (!in.read(reinterpret_cast<char*>(buf), 8)) return false;
    if (in.gcount() != 8) return false;
    v = load_little_s64(buf);
    return true;
}

} // namespace byte_io
} // namespace serialization
} // namespace protocols
} // namespace bmi
} // namespace models
