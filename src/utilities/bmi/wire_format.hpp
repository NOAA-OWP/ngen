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
INTERNAL — DO NOT INCLUDE FROM EXTERNAL CALLERS.

This header is an implementation detail of the BMI serialization
protocol library. It lives under `src/` (not `include/`) so that
external consumers — engines, tools, downstream libraries linking
against `NGen::bmi_protocols` — cannot reach into the wire format
and write arbitrary bytes that bypass the record layer's correctness
checks, version handling, and identity guarantees.

The public API for producing or consuming records lives in
`include/utilities/bmi/serialization_record.hpp`. The struct and
helpers in this file are NOT part of the library's stable API.

Visibility is enforced by include-path discipline: this header is
only on the PRIVATE include path of the `ngen_bmi_protocols`
library's own translation units and of the `test_bmi_protocols`
test target. It is never exported via `target_include_directories(PUBLIC ...)`.

------------------------------------------------------------------------
Fixed-prefix wire format for serialization records.

This header defines a compact, portable on-disk header that prefixes
every serialization record. Each field is encoded one byte at a
time in little-endian order via the helpers in `byte_io.hpp` — an
implementation detail of this header that has no API contract of
its own. Readers do not need to reach for byte_io.hpp; the wire
format's behavior, claims, and version are all documented here.

Why roll our own wire format instead of using a library
-------------------------------------------------------
The wire format does not use `boost::serialization`, protobuf,
flatbuffers, or any other serialization library. The reasons it
earns its keep:

1. **Cross-host portability**. `boost::archive::binary_oarchive`
   is documented by boost itself as "a non-portable native binary
   archive": it writes data in host-native form and stamps the
   archive with metadata describing the host's integer sizes and
   endianness. A cross-host reader detects the mismatch via that
   metadata and *throws* rather than silently misinterpreting
   bytes — better than silent misread, but still leaves the
   archive unusable across hosts with different representations.
   Boost's own remedy is an *example* file shipped alongside the
   library (`portable_binary_iarchive`) whose header explicitly
   states that it addresses "integer size and endianness so that
   binary archives can be passed across systems"; the standard
   `binary_oarchive` makes no such promise. Our field-level
   little-endian writes produce identical bytes on any host, by
   construction.

   Citations:
       https://www.boost.org/doc/libs/release/libs/serialization/doc/archives.html
         ("Archive Models" section — binary_oarchive and
         binary_iarchive are described there as "a non-portable
         native binary archive").
       https://www.boost.org/doc/libs/release/libs/serialization/example/portable_binary_iarchive.hpp
         (header comment of boost's own portable-binary example).

2. **Self-specified format**. A boost archive's bytes are defined
   by boost's documentation, the host's compiler, and which
   `BOOST_CLASS_VERSION` macros are visible at compile time. The
   spec lives in three places and drifts when any of them moves.
   Our format is fully specified in this header: the on-disk
   layout table below is the wire contract, period. Anyone with a
   hex editor and this comment can decode a file.

3. **External tool compatibility**. The bytes are plain
   little-endian integers; `file(1)` matchers, audit scripts in
   any language, and `xxd`/`hexdump` workflows can read them
   without a boost build.

4. **Clean truncation semantics**. boost archives raise
   `archive_exception` on every error condition: corruption
   (`invalid_signature`, `invalid_class_name`, ...), truncation
   (`input_stream_error` on a read past EOF), version mismatch
   (`unsupported_version`, `unsupported_class_version`). The
   exception types are distinguishable by name, so a careful
   caller could in principle map `input_stream_error` to "stop
   reading, the records before are fine" — but the caller has to
   wrap every read in try/catch and write that translation
   explicitly. Our reads return `false` on a short read at any
   field boundary; the caller's control flow is identical for
   clean EOF and torn-record EOF, no exception machinery required.
   See https://www.boost.org/doc/libs/latest/libs/serialization/doc/exceptions.html
   for boost's full exception taxonomy.

Why field-by-field instead of a packed struct
---------------------------------------------
Even with custom I/O, the *natural* implementation would be to
declare a `__attribute__((packed))` struct and `reinterpret_cast`
it onto the stream. We don't, because: compiler padding rules,
member alignment requirements, and host endianness all conspire
to make `reinterpret_cast` over a packed struct subtly wrong on
at least some target combination. Writing each field explicitly
as little-endian bytes makes the on-disk layout identical on any
host and free of padding surprises.

On-disk layout for wire_version=1 (RecordPrefix::PREFIX_BYTES, little-endian, no padding):

    offset    size  field                  type    notes
    --------  ----  ---------------------  ------  --------------------------------
    [0..4)      4   magic                  uint32  'NGSR' bytes-on-disk (N,G,S,R)
    [4..5)      1   wire_version           uint8   prefix-layout version
    [5..13)     8   time_step              int64   simulation step counter
    [13..21)    8   simulation_timestamp   int64   sim time, signed seconds since
                                                   the Unix epoch (1970-01-01T00:00:00 UTC)
    [21..29)    8   checkpoint_epoch       int64   wall-clock event time, signed
                                                   seconds since the Unix epoch
    [29..31)    2   id_length              uint16  bytes of id that follow
    [31..39)    8   payload_length         uint64  bytes of payload that follow

Followed on the stream by:
    [id_length bytes of raw id]
    [payload_length bytes of opaque payload]

Timestamp encoding convention
-----------------------------
Both `simulation_timestamp` and `checkpoint_epoch` are **signed
seconds since the Unix epoch** (1970-01-01T00:00:00 UTC). This is
mandated by `wire_version=1`. The int64 range covers ±292 billion
years from 1970, we should be good for a while with this...

A future `wire_version` may redefine the time encoding (e.g.
milliseconds, a different reference epoch, or a non-second tick
quantum), at which point readers branch on `wire_version` to
select the parser — same mechanism that governs every other
layout-or-semantics change.

Models that genuinely need an encoding the wire format does not
support — sub-second precision, a domain-specific reference
calendar, non-Earth time — embed their own time representation
inside their payload bytes. The wire format's timestamp fields
are a coarse indexing aid for the protocol layer, not the
canonical authority on the model's notion of simulation time.

The two timestamp fields are not redundant
------------------------------------------
`simulation_timestamp` is the *simulated* time the record
represents: the moment in the model's universe being captured.
`checkpoint_epoch` is the *wall-clock* time the operator
triggered the save event. The two values diverge in every
nontrivial run:

  * Reanalysis (modeling the past): simulation_timestamp ≪
    checkpoint_epoch.
  * Forecast (modeling the future): simulation_timestamp ≫
    checkpoint_epoch.
  * Restart re-run: same simulation_timestamp on the new record,
    later checkpoint_epoch.

Carrying both per-record makes records self-describing — any
single record can answer both "what simulated moment does this
represent?" and "when did the operator save this?" without
needing to consult the surrounding filesystem layout (which not
every backend preserves identically across copy, concatenation,
or format migration).

wire_version
------------
The prefix carries exactly one version field, `wire_version`,
which covers the prefix-layout itself: if any field is added,
removed, reordered, widened, or has its semantic meaning change
(timestamp encoding, reference epoch, etc.), `wire_version`
bumps and readers select the appropriate parser. The v0.1 design
also had a payload-schema version byte; that was meaningful when
the record was a boost archive the library could introspect, but
it has no role here — the new format treats the payload as
opaque bytes, so the library can neither enforce nor detect a
payload-schema mismatch. Model authors who want payload
versioning embed their own header inside the payload bytes (see
the "Recommended: defensive payload format" section of
BMI_SERIALIZATION_PROTOCOL.md); the wire format does not pretend
to provide it for them.

Signed integer fields and the two's-complement assumption
---------------------------------------------------------
Three of the prefix's fields are signed int64 (`time_step`,
`simulation_timestamp`, `checkpoint_epoch`). The on-disk bytes
for a signed value are written as the bit pattern of the
matching unsigned width, transferred via `std::memcpy`. This
encoding assumes the host uses **two's-complement signed
integer representation**.

The assumption matters for *cross-host* readability between
different signed representations — same-host round-trip works
regardless of representation because memcpy preserves the bit
pattern. ngen does not target hosts with non-two's-complement
signed integers: C++17 (ngen's required standard) does not
formally mandate two's complement, but every architecture and
compiler in ngen's CI matrix (and in every commercial deployment
target) uses it.

**Operational canary**: two tests in `test_bmi_protocols` verify
the assumption holds on the host that runs the test binary:

    ByteIO.i32_explicit_byte_layout_negative
    ByteIO.i64_explicit_byte_layout_negative

Both check that `int{32,64}_t{-1}` writes as all-0xFF bytes on
disk, which is true iff the running host uses two's complement.
If those tests fail on a target, the wire format's signed-integer
fields cannot be safely written or read on that target.

The tests validate the *run* host, not the *build* host. In CI
those are the same machine, but for cross-compiled artifacts or
for production binaries deployed to hosts other than the CI
runner, operators wanting strong guarantees should run
test_bmi_protocols on the deployment host before trusting records
produced or restored there.

Truncation tolerance
--------------------
Any short read inside the prefix returns `false` from
`read_record_prefix`. Short reads in the id or payload bytes
that follow the prefix are the caller's responsibility — the
prefix says how many bytes to expect, and the caller decides
how to interpret a truncated trailer. Magic-byte mismatch is
the signal for "this is not a record start"; the prefix layer
treats it as a successful read of garbage values, leaving
validation to the caller.

Cross-host portability — what the format does and does not claim
----------------------------------------------------------------
The prefix's signed and unsigned integer fields are portable
across any two two's-complement little-endian-or-big-endian hosts
— the field-level LE encoding produces identical bytes on any
host, and the two's-complement canary above verifies the signed
assumption at run time on each deployment.

The payload bytes that follow the prefix are **opaque to the
wire format**. The library never interprets them, never re-encodes
them, never checks them against a representation assumption. A
model that writes raw signed integers, host-endian-encoded
values, or any other representation-dependent bit pattern into
its payload has its own portability story to manage; the wire
format makes no claims about it. A record is portable as a whole
iff *both* the prefix (covered here) and the payload format (the
model's responsibility) survive the cross-host trip. Model
authors who want payload portability are pointed at the
"Recommended: defensive payload format" section of
BMI_SERIALIZATION_PROTOCOL.md.
*/
#pragma once

#include "byte_io.hpp"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>

namespace models {
namespace bmi {
namespace protocols {
namespace serialization {
namespace wire_format {

/** @brief Outcome of a wire-format read operation.
 *
 *  `read_record_prefix` returns this directly: `Ok` if a full
 *  prefix arrived, `Eof` if the stream ended (cleanly or torn)
 *  before the prefix completed. There is no third "error" state
 *  at this layer because the wire-format primitive does not
 *  validate the bytes — magic / wire_version / payload-cap
 *  checks live at the validation layer in
 *  `serialization_record.hpp`, which returns
 *  `expected<Status, std::string>` and reserves the error arm
 *  for malformed records.
 *
 *  Sub-byte primitives (`byte_io::read_*`) still return `bool` —
 *  they are an implementation detail of the wire format with no
 *  meaningful three-state distinction. */
enum class Status { Ok, Eof };

/** @brief Fixed-prefix wire header for a serialization record.
 *
 * The struct is an in-memory mirror of the on-disk header. It is
 * NEVER written or read via reinterpret_cast — every field flows
 * through the little-endian helpers in byte_io.hpp (this header's
 * private implementation detail), so neither compiler padding nor
 * host endianness affects the on-disk bytes.
 */
struct RecordPrefix {
    /** 'NGSR' written byte-first. The on-disk bytes are
     *  N(0x4E), G(0x47), S(0x53), R(0x52), which read back as
     *  little-endian uint32 0x5253474E.
     *
     *  A hex dump of any record file therefore starts with the
     *  ASCII text "NGSR" — convenient for file-type identification
     *  by tools that aren't this code. */
    static constexpr uint32_t MAGIC = 0x5253474Eu;

    /** The wire-format version this code writes. Bump if the prefix
     *  gains, loses, or reorders fields, or if the semantic meaning
     *  of any existing field changes (e.g. a switch from
     *  seconds-since-Unix-epoch to milliseconds). Readers must select
     *  the parser matching the wire_version they observe in a record;
     *  this constant is the value the writer stamps on new records.
     *
     *  Naming convention: the plain VERSION constant always tracks
     *  the newest version. Historical values, when they exist, are
     *  retained as version-tagged named constants (VERSION_V1 = 1,
     *  VERSION_V2 = 2, ...) so reader branches can name them stably
     *  without confusion — VERSION moves over time, VERSION_VN
     *  never does. v1 is the first version, so no historical
     *  constants exist yet. */
    static constexpr uint8_t VERSION = 1;

    /** Fixed prefix size on disk for the current wire_version,
     *  computed from the field widths below. No compiler padding is
     *  involved because every byte is field-by-field written.
     *  Callers that need to skip past the prefix (e.g., index
     *  builders, position computations) read this constant rather
     *  than inlining its current value — a future wire_version bump
     *  may grow the prefix, and consumers that hard-code the size
     *  will silently misalign. */
    static constexpr std::size_t PREFIX_BYTES = sizeof(uint32_t) // magic
                                                + sizeof(uint8_t) // wire_version
                                                + sizeof(int64_t) // time_step
                                                + sizeof(int64_t) // simulation_timestamp
                                                + sizeof(int64_t) // checkpoint_epoch
                                                + sizeof(uint16_t) // id_length
                                                + sizeof(uint64_t); // payload_length

    uint32_t magic               = MAGIC;
    uint8_t wire_version         = VERSION;
    int64_t time_step            = 0;
    int64_t simulation_timestamp = 0;
    int64_t checkpoint_epoch     = 0;
    uint16_t id_length           = 0;
    uint64_t payload_length      = 0;
};

/** @brief Encode a `RecordPrefix` into the @p out buffer using
 *  field-by-field little-endian encoding. The buffer MUST be at
 *  least `RecordPrefix::PREFIX_BYTES` bytes long; exactly
 *  `PREFIX_BYTES` bytes are written.
 *
 *  This is the **source of truth** for the on-disk prefix layout.
 *  Stream and direct-fd writers both go through it, so the encoded
 *  bytes are byte-identical regardless of how the caller delivers
 *  them to the underlying medium.
 */
inline void encode_record_prefix(std::uint8_t* out, const RecordPrefix& p) {
    // byte encode each field, advancing the pointer as we go.
    byte_io::store_little_u32(out, p.magic);
    out += sizeof(p.magic);
    byte_io::store_little_u8(out, p.wire_version);
    out += sizeof(p.wire_version);
    byte_io::store_little_s64(out, p.time_step);
    out += sizeof(p.time_step);
    byte_io::store_little_s64(out, p.simulation_timestamp);
    out += sizeof(p.simulation_timestamp);
    byte_io::store_little_s64(out, p.checkpoint_epoch);
    out += sizeof(p.checkpoint_epoch);
    byte_io::store_little_u16(out, p.id_length);
    out += sizeof(p.id_length);
    byte_io::store_little_u64(out, p.payload_length);
    out += sizeof(p.payload_length);
}

/** @brief Decode a `RecordPrefix` from the @p in buffer. The buffer
 *  MUST be at least `RecordPrefix::PREFIX_BYTES` bytes long.
 *
 *  Magic and wire-version validation are the caller's responsibility
 *  — this function only decodes the fields. */
inline void decode_record_prefix(const std::uint8_t* in, RecordPrefix& p) {
    // byte decode each field, advancing the pointer as we go.
    p.magic = byte_io::load_little_u32(in);
    in += sizeof(p.magic);
    p.wire_version = byte_io::load_little_u8(in);
    in += sizeof(p.wire_version);
    p.time_step = byte_io::load_little_s64(in);
    in += sizeof(p.time_step);
    p.simulation_timestamp = byte_io::load_little_s64(in);
    in += sizeof(p.simulation_timestamp);
    p.checkpoint_epoch = byte_io::load_little_s64(in);
    in += sizeof(p.checkpoint_epoch);
    p.id_length = byte_io::load_little_u16(in);
    in += sizeof(p.id_length);
    p.payload_length = byte_io::load_little_u64(in);
    in += sizeof(p.payload_length);
}

/** @brief Write a `RecordPrefix` to @p out by writing each field
 *  through `byte_io`'s stream adapters. Exactly `PREFIX_BYTES`
 *  bytes are emitted.
 *
 *  The caller is responsible for following the prefix on the stream
 *  with `id_length` bytes of id and `payload_length` bytes of
 *  payload — the prefix carries the lengths but does not carry the
 *  bodies. */
inline void write_record_prefix(std::ostream& out, const RecordPrefix& p) {
    byte_io::write_u32_le(out, p.magic);
    byte_io::write_u8(out, p.wire_version);
    byte_io::write_i64_le(out, p.time_step);
    byte_io::write_i64_le(out, p.simulation_timestamp);
    byte_io::write_i64_le(out, p.checkpoint_epoch);
    byte_io::write_u16_le(out, p.id_length);
    byte_io::write_u64_le(out, p.payload_length);
}

/** @brief Read a `RecordPrefix` from @p in by reading each field
 *  through `byte_io`'s stream adapters. Returns `Status::Ok` on a
 *  full read; `Status::Eof` if the stream ended before the prefix
 *  completed (EOF at start, or short read mid-prefix).
 *
 *  Magic and wire-version validation are the caller's responsibility
 *  — this function reports only "did a full prefix arrive." A reader
 *  that needs to distinguish "torn file at the prefix boundary" from
 *  "this is not a record" inspects `magic` and `wire_version` after
 *  a successful read. */
inline Status read_record_prefix(std::istream& in, RecordPrefix& p) {
    if (!byte_io::read_u32_le(in, p.magic)) return Status::Eof;
    if (!byte_io::read_u8(in, p.wire_version)) return Status::Eof;
    if (!byte_io::read_i64_le(in, p.time_step)) return Status::Eof;
    if (!byte_io::read_i64_le(in, p.simulation_timestamp)) return Status::Eof;
    if (!byte_io::read_i64_le(in, p.checkpoint_epoch)) return Status::Eof;
    if (!byte_io::read_u16_le(in, p.id_length)) return Status::Eof;
    if (!byte_io::read_u64_le(in, p.payload_length)) return Status::Eof;
    return Status::Ok;
}

} // namespace wire_format
} // namespace serialization
} // namespace protocols
} // namespace bmi
} // namespace models
