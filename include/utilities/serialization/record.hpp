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

#include <cctype>
#include <cstdint>
#include <ctime>
#include <exception>
#include <limits>
#include <string>
#include <vector>

namespace ngen{ namespace serialization{
/**
 *  Tests in `parse_timestamp_Test.cpp` pin both
 *  `TIMESTAMP_STRPTIME_FORMAT` and
 *  `UNPARSEABLE_TIMESTAMP_SENTINEL`
 *  so producer/consumer drift surfaces as a test failure.
 */

/** @brief `strptime` format used by `parse_timestamp`'s formatted-
 *  string fallback.
 */
constexpr const char* TIMESTAMP_STRPTIME_FORMAT = "%Y-%m-%d %T";

/** @brief Out-of-band value returned by `parse_timestamp` and its
 *  helpers when the input string cannot be resolved to a Unix epoch.
 *
 *  INT64_MIN interpreted as Unix epoch is roughly 9.2e18 seconds
 *  before 1970 — far enough before any plausible simulated moment
 */
constexpr int64_t UNPARSEABLE_TIMESTAMP_SENTINEL
    = (std::numeric_limits<int64_t>::min)();

/** @brief Parse @p s as a whole-string signed integer interpreted
 *  as Unix epoch seconds (e.g. "1448931600", "-3786825600",
 *  "  42  "). Leading/trailing whitespace is stripped before
 *  conversion. Any other form is considered unparseable.
 *
 *  Returns `UNPARSEABLE_TIMESTAMP_SENTINEL` on any failure
 *  (partial match, empty, whitespace-only, non-numeric, integer
 *  overflow). Never throws.
 */
inline int64_t parse_epoch_string(const std::string& s) {
    try {
        std::size_t     consumed = 0;
        const long long v        = std::stoll(s, &consumed);
        while (consumed < s.size()
               && std::isspace(static_cast<unsigned char>(s[consumed]))) {
            ++consumed;
        }
        if (consumed == s.size()) {
            return static_cast<int64_t>(v);
        }
    } catch (const std::exception&) {
        // std::out_of_range / std::invalid_argument — fall through
        // to the sentinel.
    }
    return UNPARSEABLE_TIMESTAMP_SENTINEL;
}

/** @brief Parse @p s as a formatted timestamp matching
 *  `TIMESTAMP_STRPTIME_FORMAT`. Interpreted in UTC via `timegm`.
 *  White space is allowed and doesn't impact the match.
 *
 *  Returns `UNPARSEABLE_TIMESTAMP_SENTINEL` on any failure
 *  (empty, whitespace-only, partial match, trailing non-whitespace).
 */
inline int64_t parse_formatted_time(const std::string& s) {
    std::tm     tm  = {};
    const char* end = ::strptime(s.c_str(), TIMESTAMP_STRPTIME_FORMAT, &tm);
    if (end != nullptr) {
        while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end))) {
            ++end;
        }
        if (*end == '\0') {
            return static_cast<int64_t>(::timegm(&tm));
        }
    }
    return UNPARSEABLE_TIMESTAMP_SENTINEL;
}

/** @brief Parse @p s as seconds-since-Unix-epoch.
 *
 *  Tries `parse_epoch_string` first, then attempts
 *  `parse_formatted_time` (`TIMESTAMP_STRPTIME_FORMAT` match).
 *  Returns `UNPARSEABLE_TIMESTAMP_SENTINEL` only when neither
 *  parser can resolve the input.
 */
inline int64_t parse_timestamp(const std::string& s) {
    if (s.empty()) return UNPARSEABLE_TIMESTAMP_SENTINEL;
    const int64_t epoch = parse_epoch_string(s);
    if (epoch != UNPARSEABLE_TIMESTAMP_SENTINEL) return epoch;
    return parse_formatted_time(s);
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
