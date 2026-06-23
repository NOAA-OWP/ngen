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
The value-type contract shared between record producers and
record-storage backends.

A `Record` (in the `ngen::serialization` namespace) is the
engine-agnostic interchange unit between code that *produces* a
piece of saved state (e.g. a BMI adapter capturing a model's
bytes) and code that *stores* that state (e.g. a file backend
that lays it out on disk, an HDF5 backend that stores it as a
dataset + attributes, or any future storage).

This header defines only the value type and a small parse helper.
It does NOT prescribe a wire format — backends are free to encode
the fields however suits their storage.
*/
#pragma once

#include <boost/core/span.hpp>

#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace ngen{ namespace serialization{

/** @brief Parse a timestamp string into the int64 second-since-epoch
 *  slot used by the record's timestamp fields.
 *
 *  The engine hands serialization code simulation timestamps as
 *  opaque strings; the record value type wants compact fixed-width
 *  integers. Callers whose strings are already `strtoll`-parseable
 *  (e.g. seconds since epoch) get a one-to-one mapping; callers with
 *  formatted strings that don't parse cleanly get 0, and the record
 *  is still produced — restore-by-timestamp just won't work against
 *  those records unless the caller matches on the same "0" sentinel.
 *  This is a deliberate policy: a parse failure is not fatal to
 *  save.
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

/** @brief One checkpoint record: a saved-state blob tagged with its
 * feature id, simulation step counter, the simulated moment the
 * state represents, and the wall-clock moment the operator
 * triggered the save.
 *
 * `simulation_timestamp` and `checkpoint_epoch` are signed seconds
 * since the Unix epoch (1970-01-01T00:00:00 UTC). `payload` is
 * opaque bytes — neither this value type nor any storage backend
 * interprets them; the producer chooses the byte format and the
 * eventual consumer (typically the same producer at restore time)
 * is responsible for parsing them.
 *
 * `checkpoint_epoch` defaults to 0 in the constructor. Producers
 * that want a wall-clock stamp set it before handing the record
 * to a backend; the BMI protocol library's save path uses
 * `std::time(nullptr)`.
 */
namespace detail {

// Shared field set for the two public checkpoint-record types
// (`Record` and `RecordView`, below). Implementation alignment
// mechanism only — adding or changing a metadata field touches
// one definition and both aliases inherit the change.
template <typename PayloadT>
struct RecordT {
    std::string       id;
    int64_t           time_step            = 0;
    int64_t           simulation_timestamp = 0;
    int64_t           checkpoint_epoch     = 0;
    PayloadT          payload{};

    RecordT() = default;
    RecordT(std::string id_,
                        int64_t time_step_,
                        int64_t simulation_timestamp_,
                        PayloadT payload_,
                        int64_t checkpoint_epoch_ = 0)
        : id(std::move(id_))
        , time_step(time_step_)
        , simulation_timestamp(simulation_timestamp_)
        , checkpoint_epoch(checkpoint_epoch_)
        , payload(std::move(payload_)) {}
};

} // namespace detail

/** @brief A checkpoint record that owns its payload.
 *
 *  `payload` is a `std::vector<char>` of opaque saved bytes —
 *  neither this value type nor any consumer interprets them; the
 *  producer chooses the byte format and the eventual consumer
 *  parses them.
 *
 *  See `RecordView` for the non-owning payload-view.
 */
using Record     = detail::RecordT<std::vector<char>>;

/** @brief A checkpoint record that views its payload (non-owning).
 *
 *  Same metadata field set as `Record`, but `payload` is a
 *  `boost::span<const char>` rather than an owning vector. The
 *  caller keeps the payload bytes alive elsewhere for the
 *  lifetime of the view.
 *
 */
using RecordView = detail::RecordT<boost::span<const char>>;

}}  // namespace ngen::serialization
