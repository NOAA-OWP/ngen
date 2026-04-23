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
Version 0.1
Shared on-disk record format for ngen BMI state serialization.

On-disk layout
--------------
A checkpoint file is a concatenation of length-prefixed records:

    [ uint64_t record_length ][ <record_length> bytes of boost binary archive ]
    [ uint64_t record_length ][ <record_length> bytes of boost binary archive ]
    ...

The length prefix enables three things that matter at scale:
  1. Truncation tolerance — a torn final record (length header present but
     fewer than record_length bytes following) is detected and treated as
     end-of-stream, not as a corrupt file.
  2. Cheap skipping when building an index — `CheckpointIndex` (see
     deserialization.cpp) can seek past record payloads without running a
     boost archive read, which turns the index build from O(N * payload)
     into O(N * sizeof(header) + N * seek).
  3. Append-only multi-writer safety — because each record is a self-
     contained boost archive, and the length prefix lets a reader locate
     the next record without reconstructing archive state, multiple writers
     can append independently without file-level coordination. Writers
     must still serialize their writes relative to each other — see
     NgenSerializationProtocol for the per-instance mutex policy.

Schema versioning — two concerns, two mechanisms
------------------------------------------------
There are two orthogonal "version" numbers in play; keep them straight
when the format changes:

  * BOOST_CLASS_VERSION (below) drives the boost::serialization library's
    own wire-format evolution. Bumping it and branching inside serialize()
    on `archive_version` lets us read old archives written before a
    library-level change (e.g. adding or reordering fields in this
    struct). Boost writes this version into each archive header — it's
    archive-scoped, not record-scoped.
  * SerializationRecord::CURRENT_VERSION is our application-level schema
    stamp. It's serialized as the first field of every record, so a
    reader can inspect it directly without relying on boost archive
    internals. Use this when changing record *semantics* that boost
    alone can't describe — e.g. payload interpretation rules, timestamp
    encoding conventions, or compatibility with older restore logic.

The two version numbers are independent — either may advance without
the other. Bump `BOOST_CLASS_VERSION` for wire-format changes (new
fields, reordering), and bump `CURRENT_VERSION` for semantic changes
the wire-format version can't describe. Branch explicitly on
`archive_version` inside `serialize()` or on `rec.version` at the
read site, whichever matches the kind of change you're making.

Uniqueness
----------
Each (id, time_step) pair is unique within a file. One entity may write
multiple records at different time steps, but only one record per step.
*/
#pragma once

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>

#include <cstdint>
#include <exception>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace models{ namespace bmi{ namespace protocols{

/** @brief Size, in bytes, of the per-record length prefix on disk.
 *
 * Single source of truth for the record header layout — every place in
 * the code that seeks past or otherwise accounts for the prefix reads
 * this constant rather than inlining `sizeof(uint64_t)`. If the header
 * ever grows (e.g. gains a magic number or a flags word), update this
 * one value and both the writer and the index-based readers will stay
 * in sync.
 */
constexpr std::size_t RECORD_LENGTH_PREFIX_BYTES = sizeof(uint64_t);

/** @brief Sanity cap on a single record's archive body size.
 *
 * Defense in depth against corrupted or tampered on-disk length
 * prefixes: a byte-flipped `uint64_t` could otherwise drive
 * `read_next_record` into a multi-gigabyte allocation. Real BMI
 * state blobs are nowhere near this bound — 1 GiB is already far
 * larger than any plausible single-entity state — so false positives
 * are not a concern; the value exists to reject obvious garbage.
 *
 * Readers that hit this cap raise an exception rather than return
 * cleanly: the condition is "the file says something impossible",
 * not "end of legitimate content".
 */
constexpr uint64_t MAX_RECORD_ARCHIVE_BYTES = 1ULL << 30;  // 1 GiB

/** @brief Parse a `Context::timestamp` string into the on-disk `int64_t`
 * timestamp slot.
 *
 * The engine hands BMI protocol code simulation timestamps as opaque
 * strings; the record layer wants compact fixed-width integers.
 * Callers whose strings are already `strtoll`-parseable (e.g. seconds
 * since epoch) get a one-to-one mapping; callers with formatted strings
 * that don't parse cleanly get 0, and the record is still written —
 * restore-by-timestamp just won't work against those records unless
 * the caller matches on the same "0" sentinel. This is a deliberate
 * policy: a parse failure is not fatal to save.
 *
 * Kept here, next to the record struct that ultimately stores the
 * value, so the save and restore protocols use the exact same rule.
 */
inline int64_t parse_timestamp(const std::string& s) {
    if (s.empty()) return 0;
    try {
        std::size_t consumed = 0;
        const long long v = std::stoll(s, &consumed);
        // Require at least one digit to have been consumed — a "t0"-
        // style label stops at 'not a digit' after consuming zero
        // characters, which we treat as unparseable.
        if (consumed == 0) return 0;
        return static_cast<int64_t>(v);
    } catch (const std::exception&) {
        return 0;
    }
}

/** @brief One checkpoint record: a BMI model's state bytes tagged with its
 * feature id, the simulation time step, and the simulation timestamp (as
 * seconds since the Unix epoch) at capture.
 *
 * The `timestamp` field is an `int64_t` rather than a formatted string:
 * the protocol is not concerned with human readability at the storage
 * layer, and a fixed 8-byte timestamp keeps record headers predictable
 * enough to support fast index scans over files containing millions of
 * records. Callers whose upstream `Context::timestamp` is a formatted
 * string are responsible for parsing it to an epoch value (or any other
 * monotone integer encoding they choose) before record construction;
 * see `NgenSerializationProtocol` for the reference conversion.
 */
struct SerializationRecord {
    /** Current application-level schema version. Bump and wire a branch
     *  in `serialize()` (keyed on `rec.version`, not `archive_version`)
     *  when record semantics change. `uint8_t` is portable across
     *  platforms and generous — 255 schema revisions will outlast us. */
    static constexpr uint8_t CURRENT_VERSION = 1;

    // Field order below is chosen for readability ("version, then id,
    // then when / what") rather than for alignment-minimal padding.
    // The struct is materialized one at a time during save/restore and
    // is never held in bulk — the process-global state lives in
    // CheckpointIndex's flat IndexEntry (no std::string). The on-disk
    // format is controlled entirely by `serialize()` below and is
    // unaffected by whatever padding the compiler chooses here.
    uint8_t           version   = CURRENT_VERSION;
    std::string       id;
    int               time_step = 0;
    int64_t           timestamp = 0;
    std::vector<char> payload;

    SerializationRecord() = default;
    SerializationRecord(std::string id_,
                        int time_step_,
                        int64_t timestamp_,
                        std::vector<char> payload_)
        : id(std::move(id_))
        , time_step(time_step_)
        , timestamp(timestamp_)
        , payload(std::move(payload_)) {}

    template <class Archive>
    void serialize(Archive& ar, const unsigned int archive_version) {
        ar & version;
        ar & id;
        ar & time_step;
        ar & timestamp;
        ar & payload;
    }
};

/** @brief Append a length-prefixed, self-contained boost binary archive
 * holding @p rec to @p out.
 *
 * The record is serialized into a memory buffer first so the exact byte
 * length is known before any bytes hit @p out. The length is written as
 * a little-endian-on-little-endian-hosts (natively written) `uint64_t`;
 * on-disk portability is assumed between hosts with matching endianness,
 * which matches the boost archive's own assumption. The archive
 * destructor flushes its header + payload into the memory buffer before
 * the length is measured and emitted.
 *
 * @throws boost::archive::archive_exception if serialization fails.
 */
inline void write_record(std::ostream& out, const SerializationRecord& rec) {
    std::stringstream buf(std::ios::out | std::ios::in | std::ios::binary);
    {
        boost::archive::binary_oarchive oa(buf);
        oa << rec;
    } // archive destructor flushes here — buf now holds the complete archive
    const std::string bytes = buf.str();
    const uint64_t length = static_cast<uint64_t>(bytes.size());
    static_assert(sizeof(length) == RECORD_LENGTH_PREFIX_BYTES,
                  "length-prefix width must match RECORD_LENGTH_PREFIX_BYTES");
    out.write(reinterpret_cast<const char*>(&length), RECORD_LENGTH_PREFIX_BYTES);
    out.write(bytes.data(), bytes.size());
}

/** @brief Read a length prefix and return the record's payload length
 * without deserializing the archive.
 *
 * Used by index builders that need to walk the file quickly to record
 * (offset, id) pairs, and internally by `read_next_record` to keep the
 * EOF / torn-header policy in one place.
 *
 * @param in  Input stream positioned at the start of a length prefix or
 *            at EOF.
 * @param length_out Output parameter populated on success with the
 *                   payload length (not including the 8-byte header).
 * @return true if a full length header was read; false on EOF or a
 *         truncated header.
 */
inline bool read_record_length(std::istream& in, uint64_t& length_out) {
    if (in.peek() == std::istream::traits_type::eof()) {
        return false;
    }
    static_assert(sizeof(length_out) == RECORD_LENGTH_PREFIX_BYTES,
                  "length-prefix width must match RECORD_LENGTH_PREFIX_BYTES");
    return static_cast<bool>(in.read(reinterpret_cast<char*>(&length_out),
                                     RECORD_LENGTH_PREFIX_BYTES));
}

/** @brief Read the next record from @p in.
 *
 * @param in  Input stream positioned at the start of a length prefix (or
 *            at EOF).
 * @param rec Output parameter populated on success.
 * @return true if a record was read; false if the stream was at EOF
 *         before reading began, OR if the stream ended mid-record
 *         (truncation). Torn files therefore stop cleanly at the last
 *         intact record rather than throwing — an important property
 *         for restart-from-crash workflows.
 *
 * @throws std::runtime_error if the length prefix reports a byte count
 *         larger than `MAX_RECORD_ARCHIVE_BYTES` — that means the file
 *         is corrupted or tampered with, not merely torn.
 * @throws boost::archive::archive_exception if a full record was read
 *         but failed to deserialize (corruption rather than truncation).
 */
inline bool read_next_record(std::istream& in, SerializationRecord& rec) {
    // Shared helper with index builders: single source of truth for how
    // the 8-byte length prefix is read and how EOF / torn-header is
    // distinguished from a valid zero-length prefix. Returning false here
    // covers both clean end-of-stream and a half-written header.
    uint64_t length = 0;
    if (!read_record_length(in, length)) {
        return false;
    }
    // Reject pathologically-large lengths before attempting to allocate
    // or read that many bytes. See MAX_RECORD_ARCHIVE_BYTES above.
    if (length > MAX_RECORD_ARCHIVE_BYTES) {
        throw std::runtime_error(
            "serialization_record: length prefix " + std::to_string(length) +
            " exceeds MAX_RECORD_ARCHIVE_BYTES sanity cap");
    }
    // Read the exact number of archive bytes into a buffer. A short read
    // means the tail of the file was truncated; surface that as
    // end-of-stream rather than an archive exception, so a partially-
    // written file from a crashed writer still yields every record that
    // was fully flushed before the crash.
    std::string bytes;
    bytes.resize(static_cast<size_t>(length));
    if (length > 0) {
        in.read(&bytes[0], static_cast<std::streamsize>(length));
        if (static_cast<uint64_t>(in.gcount()) != length) {
            return false;
        }
    }
    // At this point a valid record always contains a non-empty boost
    // archive (header + payload). length == 0 means the file reported
    // no archive body — constructing the binary_iarchive on an empty
    // stringstream throws a `boost::archive::archive_exception` when
    // it tries to read the archive's own header signature, which the
    // caller surfaces as a corrupted-record error.
    std::stringstream buf(bytes, std::ios::in | std::ios::binary);
    boost::archive::binary_iarchive ia(buf);
    ia >> rec;
    return true;
}

}}} // end namespace models::bmi::protocols

// BOOST_CLASS_VERSION and CURRENT_VERSION are intentionally independent.
// They track orthogonal concerns (see the file header's "Schema
// versioning" section for the full story):
//
//   * BOOST_CLASS_VERSION drives boost::serialization's archive wire
//     format. Bump it when adding, removing, or reordering fields on
//     the struct and branch on `archive_version` inside serialize() to
//     read old archives.
//
//   * CURRENT_VERSION stamps every record with the application schema
//     in use at write time. Bump it when record *semantics* change —
//     e.g. a new payload interpretation, a different timestamp encoding
//     convention, or compatibility rules with older restore logic.

BOOST_CLASS_VERSION(models::bmi::protocols::SerializationRecord, 1)
