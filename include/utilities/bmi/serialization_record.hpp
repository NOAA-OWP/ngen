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
Version 0.2
Shared on-disk record format for ngen BMI state serialization.

This header is the public API for producing and consuming
serialization records. The on-disk layout is the responsibility of
the internal `wire_format.hpp`, which lives under
`src/utilities/bmi/` and is not exported. External code calls into
this header's functions:

    write_record(out, rec)               — append a record
    read_next_record(in, rec)            — read the next record
    read_record_length(in, body_out)     — read prefix; report body length
    read_record_metadata(in, prefix, id) — read prefix + id; skip payload

The value type `SerializationRecord` itself lives in the engine-
agnostic `ngen::serialization` library (see
`include/utilities/serialization/record.hpp`) and is
re-exported under `models::bmi::protocols` for source compatibility
with existing call sites. Future storage backends share the same
value type without depending on the BMI library.

On-disk layout
--------------
A checkpoint file is a concatenation of records. Each record is:

    [ fixed-size prefix ][ id_length bytes of id ][ payload_length bytes of payload ]

The prefix layout is defined in `wire_format.hpp` and includes the
magic bytes "NGSR" (which identify the file format), a wire-format
version, simulation step / timestamp / wall-clock-epoch metadata,
and the two body-length fields. Anyone with a hex editor can
identify a checkpoint file by its "NGSR..." prefix.

Replaces the v0.1 design where each record was a length-prefixed
boost::serialization archive. See `wire_format.hpp` for the
rationale behind the change (cross-host portability, library
independence, index-build performance, etc.).

Truncation tolerance
--------------------
A torn final record (writer crashed mid-write, leaving fewer than
the expected bytes on disk) is detected and treated as
end-of-stream rather than as a corrupt file. The read functions
return `false` on a short read at any field boundary — the prefix,
the id bytes, or the payload bytes. This matches the v0.1 behavior
and is what restart-from-crash workflows depend on.

Uniqueness
----------
Each `(id, time_step)` pair is unique within a file. One entity may
write multiple records at different time steps, but only one
record per step.
*/
#pragma once

#include "utilities/serialization/record.hpp"
#include "wire_format.hpp"

#include <nonstd/expected.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace models {
namespace bmi {
namespace protocols {

using nonstd::expected;
using nonstd::make_unexpected;

// Bring wire_format::Status into this namespace so call sites of
// the read helpers below can write `Status::Ok` / `Status::Eof`
// without the long `serialization::wire_format::` prefix.
using serialization::wire_format::Status;

// Re-export the engine-agnostic value type and timestamp parser
// from ngen::serialization so existing call sites continue to use
// `models::bmi::protocols::SerializationRecord` / `parse_timestamp`
// unchanged. `SerializationRecordView` is the non-owning sibling
// (payload is `boost::span<const char>` rather than
// `std::vector<char>`); use it on the write path when the caller
// already holds the payload bytes in a buffer it owns for the
// duration of the call.
using SerializationRecord     = ::ngen::serialization::Record;
using SerializationRecordView = ::ngen::serialization::RecordView;
using ::ngen::serialization::parse_timestamp;

/** @brief Sanity cap on a single record's payload size.
 *
 * Defense in depth against corrupted or tampered on-disk
 * `payload_length` fields: a byte-flipped uint64 could otherwise
 * drive `read_next_record` into a multi-gigabyte allocation. Real
 * BMI state blobs are nowhere near this bound — 1 GiB is already
 * far larger than any plausible single-entity state — so false
 * positives are not a concern; the value exists to reject obvious
 * garbage.
 *
 * Readers that hit this cap surface the failure on the error arm
 * of their returned `expected<>` rather than reporting `Status::Eof`:
 * the condition is "the file says something impossible", not "end
 * of legitimate content".
 *
 * Build-time override via `-DNGEN_BMI_MAX_RECORD_PAYLOAD_BYTES=...`
 * for deployments with multi-GiB ML-state payloads. The default is
 * 1 GiB; the wire-format field type (uint64) supports values up to
 * ~18 EiB.
 */
#ifndef NGEN_BMI_MAX_RECORD_PAYLOAD_BYTES
#define NGEN_BMI_MAX_RECORD_PAYLOAD_BYTES (1ULL << 30) // 1 GiB
#endif
constexpr uint64_t MAX_RECORD_PAYLOAD_BYTES = NGEN_BMI_MAX_RECORD_PAYLOAD_BYTES;

// Internal helper shared by read_next_record, read_record_length,
// and read_record_metadata. Reads the prefix and validates the
// magic, the wire_version, and the payload_length cap. Returns
// `Status::Ok` on a fully-read valid prefix, `Status::Eof` on a
// clean short-read (EOF or torn prefix), or an error-arm string
// on any validation failure of a fully-read prefix. Internal —
// callers in this header always check the return value, so no
// nodiscard.
inline auto
read_validated_prefix(std::istream& in, serialization::wire_format::RecordPrefix& prefix)
    -> expected<Status, std::string> {
    if (const auto s = serialization::wire_format::read_record_prefix(in, prefix);
        s == Status::Eof) {
        return s; // EOF or torn prefix — clean stop, propagate
    }
    if (prefix.magic != serialization::wire_format::RecordPrefix::MAGIC) {
        return make_unexpected(
            "serialization_record: bad magic 0x" + std::to_string(prefix.magic)
            + " — file is not a v0.2 record stream or is corrupted "
              "at the record boundary"
        );
    }
    if (prefix.wire_version != serialization::wire_format::RecordPrefix::VERSION) {
        return make_unexpected(
            "serialization_record: unsupported wire_version " + std::to_string(prefix.wire_version)
            + " (this reader supports v"
            + std::to_string(serialization::wire_format::RecordPrefix::VERSION) + ")"
        );
    }
    if (prefix.payload_length > MAX_RECORD_PAYLOAD_BYTES) {
        return make_unexpected(
            "serialization_record: payload_length " + std::to_string(prefix.payload_length)
            + " exceeds MAX_RECORD_PAYLOAD_BYTES sanity cap"
        );
    }
    return Status::Ok;
}

/** @brief Append a record to @p out using the v0.2 wire format
 *  (fixed-size prefix + id bytes + payload bytes). Canonical entry
 *  point — takes a `SerializationRecordView` so callers can stream
 *  a record whose payload bytes live in a buffer they already own,
 *  without materializing a fresh owning copy.
 *
 *  Field mapping at the write site is identity: every prefix
 *  field comes directly from the matching record field. Callers
 *  who want a wall-clock stamp on `checkpoint_epoch` set it
 *  themselves (typically via `std::time(nullptr)`) before calling;
 *  default-constructed records carry `checkpoint_epoch = 0`.
 *
 *  Returns the error arm if the stream ends up in a fail state
 *  after the prefix / id / payload bytes are issued. The check
 *  is post-write — the caller doesn't need to inspect the stream
 *  itself.
 *
 *  See the overload below for the `SerializationRecord` (owning)
 *  variant; both produce identical wire output.
 */
nsel_NODISCARD inline auto write_record(std::ostream& out, const SerializationRecordView& rec)
    -> expected<void, std::string> {
    serialization::wire_format::RecordPrefix prefix;
    prefix.time_step            = rec.time_step;
    prefix.simulation_timestamp = rec.simulation_timestamp;
    prefix.checkpoint_epoch     = rec.checkpoint_epoch;
    prefix.id_length            = static_cast<uint16_t>(rec.id.size());
    prefix.payload_length       = static_cast<uint64_t>(rec.payload.size());
    // (magic and wire_version come from the struct's default values.)

    serialization::wire_format::write_record_prefix(out, prefix);
    if (!rec.id.empty()) {
        out.write(rec.id.data(), static_cast<std::streamsize>(rec.id.size()));
    }
    if (!rec.payload.empty()) {
        out.write(rec.payload.data(), static_cast<std::streamsize>(rec.payload.size()));
    }
    if (!out) {
        return make_unexpected(
            std::string("serialization_record: stream failed during record write")
        );
    }
    return {};
}

/** @brief Overload accepting an owning `SerializationRecord`.
 *
 *  Identical wire output to the `SerializationRecordView` overload
 *  above. The body mirrors the View path because RecordView's `id`
 *  field is itself an owning `std::string` — constructing a view
 *  to delegate would force an unnecessary id-string copy. The
 *  payload type is the only field where the two records diverge,
 *  and `out.write(payload.data(), payload.size())` is the same
 *  call shape for `std::vector<char>` and
 *  `boost::span<const char>`.
 */
nsel_NODISCARD inline auto write_record(std::ostream& out, const SerializationRecord& rec)
    -> expected<void, std::string> {
    serialization::wire_format::RecordPrefix prefix;
    prefix.time_step            = rec.time_step;
    prefix.simulation_timestamp = rec.simulation_timestamp;
    prefix.checkpoint_epoch     = rec.checkpoint_epoch;
    prefix.id_length            = static_cast<uint16_t>(rec.id.size());
    prefix.payload_length       = static_cast<uint64_t>(rec.payload.size());

    serialization::wire_format::write_record_prefix(out, prefix);
    if (!rec.id.empty()) {
        out.write(rec.id.data(), static_cast<std::streamsize>(rec.id.size()));
    }
    if (!rec.payload.empty()) {
        out.write(rec.payload.data(), static_cast<std::streamsize>(rec.payload.size()));
    }
    if (!out) {
        return make_unexpected(
            std::string("serialization_record: stream failed during record write")
        );
    }
    return {};
}

/** @brief Read the next record from @p in.
 *
 *  @return `Status::Ok` if a full record was read; `Status::Eof`
 *          if the stream was at EOF before reading began OR if
 *          the stream ended mid-record (truncation in the prefix,
 *          the id, or the payload) — torn files therefore stop
 *          cleanly at the last intact record. Error arm if the
 *          prefix's magic bytes are wrong, the wire_version is
 *          unsupported, or payload_length exceeds
 *          MAX_RECORD_PAYLOAD_BYTES.
 */
nsel_NODISCARD inline auto read_next_record(std::istream& in, SerializationRecord& rec)
    -> expected<Status, std::string> {
    serialization::wire_format::RecordPrefix prefix;
    auto p = read_validated_prefix(in, prefix);
    // Propagate any non-Ok outcome unchanged — error arm or Eof.
    if (!p || p.value() == Status::Eof) return p;

    // id body
    std::string id;
    id.resize(prefix.id_length);
    if (prefix.id_length > 0) {
        in.read(&id[0], static_cast<std::streamsize>(prefix.id_length));
        if (static_cast<uint64_t>(in.gcount()) != prefix.id_length) {
            return Status::Eof; // torn id — clean stop
        }
    }

    // payload body
    std::vector<char> payload;
    payload.resize(static_cast<size_t>(prefix.payload_length));
    if (prefix.payload_length > 0) {
        in.read(payload.data(), static_cast<std::streamsize>(prefix.payload_length));
        if (static_cast<uint64_t>(in.gcount()) != prefix.payload_length) {
            return Status::Eof; // torn payload — clean stop
        }
    }

    rec.id                   = std::move(id);
    rec.time_step            = prefix.time_step;
    rec.simulation_timestamp = prefix.simulation_timestamp;
    rec.checkpoint_epoch     = prefix.checkpoint_epoch;
    rec.payload              = std::move(payload);
    return Status::Ok;
}

/** @brief Read the prefix at the current stream position and
 *  report the record's body length (id bytes + payload bytes)
 *  without reading either.
 *
 *  @param in       Input stream positioned at the start of a
 *                  record (a fixed-size prefix), or at EOF.
 *  @param body_out On `Status::Ok`, populated with the number of
 *                  bytes remaining in this record after the
 *                  prefix — i.e. `id_length + payload_length`.
 *                  A caller that wants to skip past the record
 *                  without reading any of its contents calls
 *                  `in.seekg(body_out, std::ios::cur)`.
 *  @return `Status::Ok` if a full prefix was read; `Status::Eof`
 *          on EOF or a truncated prefix; error arm on bad magic,
 *          unsupported wire_version, or payload_length exceeding
 *          MAX_RECORD_PAYLOAD_BYTES.
 */
nsel_NODISCARD inline auto read_record_length(std::istream& in, uint64_t& body_out)
    -> expected<Status, std::string> {
    serialization::wire_format::RecordPrefix prefix;
    auto p = read_validated_prefix(in, prefix);
    // Propagate any non-Ok outcome unchanged — error arm or Eof.
    if (!p || p.value() == Status::Eof) return p;
    body_out = static_cast<uint64_t>(prefix.id_length) + prefix.payload_length;
    return Status::Ok;
}

/** @brief Read the prefix and id at the current stream position
 *  and skip past the payload, leaving @p in positioned at the
 *  start of the next record (or at EOF).
 *
 *  This is the fast walker a `RecordBackend`'s index builder
 *  uses to construct an in-memory index without ever reading
 *  payload bytes — the cost per record is `O(prefix + id_size)`
 *  regardless of payload size.
 *
 *  @return `Status::Ok` if metadata was read AND the payload was
 *          successfully skipped; `Status::Eof` on EOF or any
 *          truncation; error arm on bad magic, unsupported
 *          wire_version, or payload_length exceeding
 *          MAX_RECORD_PAYLOAD_BYTES.
 */
nsel_NODISCARD inline auto read_record_metadata(
    std::istream& in,
    serialization::wire_format::RecordPrefix& prefix_out,
    std::string& id_out
) -> expected<Status, std::string> {
    auto p = read_validated_prefix(in, prefix_out);
    // Propagate any non-Ok outcome unchanged — error arm or Eof.
    if (!p || p.value() == Status::Eof) return p;

    id_out.resize(prefix_out.id_length);
    if (prefix_out.id_length > 0) {
        in.read(&id_out[0], static_cast<std::streamsize>(prefix_out.id_length));
        if (static_cast<uint64_t>(in.gcount()) != prefix_out.id_length) {
            return Status::Eof; // torn id — clean stop
        }
    }

    if (prefix_out.payload_length > 0) {
        in.seekg(static_cast<std::streamoff>(prefix_out.payload_length), std::ios::cur);
        if (!in) return Status::Eof; // stream went bad while seeking
    }
    return Status::Ok;
}

} // namespace protocols
} // namespace bmi
} // namespace models
