/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Tests for the three timestamp-parsing entry points in record.hpp:

  * `parse_epoch_string`  — whole-string signed integer interpreted
                             as Unix epoch seconds. Returns
                             `UNPARSEABLE_TIMESTAMP_SENTINEL` on any
                             failure (partial match, empty,
                             whitespace-only, non-numeric, integer
                             overflow). Never throws.
  * `parse_formatted_time` — whole-string `strptime` match against
                             `TIMESTAMP_STRPTIME_FORMAT`. Returns
                             the sentinel on any non-match. Never
                             throws.
  * `parse_timestamp`     — orchestrator that tries the epoch
                             parser first and cascades to the
                             formatted parser when the epoch parser
                             returns the sentinel.

All three helpers share the same shape: `string -> int64_t`,
sentinel on failure. The tests below assert against
`UNPARSEABLE_TIMESTAMP_SENTINEL` directly so a change to its value
or to either parser's contract surfaces as a clear failure.
*/
#include "gtest/gtest.h"

#include "utilities/serialization/record.hpp"

#include <limits>

using ngen::serialization::parse_epoch_string;
using ngen::serialization::parse_formatted_time;
using ngen::serialization::parse_timestamp;
using ngen::serialization::TIMESTAMP_STRPTIME_FORMAT;
using ngen::serialization::UNPARSEABLE_TIMESTAMP_SENTINEL;

// =========================================================================
// parse_epoch_string — whole-string signed integer as Unix epoch seconds.
// Never throws; returns UNPARSEABLE_TIMESTAMP_SENTINEL on any failure.
// =========================================================================

TEST(parse_epoch_string, positive_epoch) {
    EXPECT_EQ(parse_epoch_string("1448931600"), int64_t{1448931600});
}

TEST(parse_epoch_string, zero_is_a_valid_value) {
    // 0 is the Unix epoch (1970-01-01T00:00:00Z) and must be
    // distinguishable from the sentinel.
    EXPECT_EQ(parse_epoch_string("0"), int64_t{0});
}

TEST(parse_epoch_string, negative_epoch) {
    EXPECT_EQ(parse_epoch_string("-3786825600"), int64_t{-3786825600}); // 1850-01-01T00:00:00Z
}

TEST(parse_epoch_string, surrounding_whitespace_is_tolerated) {
    EXPECT_EQ(parse_epoch_string("  1234  "), int64_t{1234});
}

TEST(parse_epoch_string, empty_returns_sentinel) {
    EXPECT_EQ(parse_epoch_string(""), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_epoch_string, whitespace_only_returns_sentinel) {
    // stoll throws std::invalid_argument internally; the helper
    // catches and resolves to the sentinel.
    EXPECT_EQ(parse_epoch_string("   "), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_epoch_string, pure_non_numeric_returns_sentinel) {
    EXPECT_EQ(parse_epoch_string("abc"), UNPARSEABLE_TIMESTAMP_SENTINEL);
    EXPECT_EQ(parse_epoch_string("not-a-timestamp"), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_epoch_string, partial_numeric_match_returns_sentinel) {
    // "2015-12-01 01:00:00" peels off "2015" via stoll but the rest
    // of the string is non-whitespace. parse_timestamp uses this
    // failure mode as the signal to cascade into the formatted
    // parser — regressing it breaks formatted-string handling.
    EXPECT_EQ(parse_epoch_string("2015-12-01 01:00:00"), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_epoch_string, integer_with_trailing_non_whitespace_returns_sentinel) {
    EXPECT_EQ(parse_epoch_string("1234abc"), UNPARSEABLE_TIMESTAMP_SENTINEL);
    EXPECT_EQ(parse_epoch_string("99garbage"), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_epoch_string, overflow_returns_sentinel) {
    // stoll throws std::out_of_range internally; the helper catches
    // and resolves to the sentinel.
    EXPECT_EQ(parse_epoch_string("9999999999999999999999999"),
              UNPARSEABLE_TIMESTAMP_SENTINEL);
}

// =========================================================================
// parse_formatted_time — whole-string strptime match against
// TIMESTAMP_STRPTIME_FORMAT. Never throws; sentinel on any non-match.
// =========================================================================

TEST(parse_formatted_time, ngen_simulation_time_format) {
    // 2015-12-01T01:00:00Z corresponds to epoch 1448931600.
    EXPECT_EQ(parse_formatted_time("2015-12-01 01:00:00"), int64_t{1448931600});
}

TEST(parse_formatted_time, trailing_whitespace_is_tolerated) {
    EXPECT_EQ(parse_formatted_time("2015-12-01 01:00:00   "), int64_t{1448931600});
}

TEST(parse_formatted_time, trailing_non_whitespace_returns_sentinel) {
    // strptime by itself doesn't require full-string consumption —
    // this guards against silently accepting "format + junk".
    EXPECT_EQ(parse_formatted_time("2015-12-01 01:00:00 extra"),
              UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_formatted_time, partial_format_match_returns_sentinel) {
    // Missing the " HH:MM:SS" portion.
    EXPECT_EQ(parse_formatted_time("2025-12-19"), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_formatted_time, pure_non_format_input_returns_sentinel) {
    EXPECT_EQ(parse_formatted_time("not-a-timestamp"), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_formatted_time, numeric_epoch_returns_sentinel) {
    // This helper is specifically the formatted parser; numeric
    // epochs are parse_epoch_string's job. strptime starts trying
    // to match %Y, gets "1448", expects '-', sees '9' — sentinel.
    EXPECT_EQ(parse_formatted_time("1448931600"), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

// =========================================================================
// parse_timestamp — orchestrator. Tries parse_epoch_string first;
// cascades to parse_formatted_time when the epoch parser returns the
// sentinel.
// =========================================================================

TEST(parse_timestamp, empty_returns_sentinel) {
    EXPECT_EQ(parse_timestamp(""), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_timestamp, whitespace_only_returns_sentinel) {
    EXPECT_EQ(parse_timestamp("   "), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_timestamp, pure_garbage_returns_sentinel) {
    EXPECT_EQ(parse_timestamp("not-a-timestamp"), UNPARSEABLE_TIMESTAMP_SENTINEL);
    EXPECT_EQ(parse_timestamp("abc"), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_timestamp, leading_integer_with_trailing_junk_returns_sentinel) {
    // epoch parser returns the sentinel here, the formatted parser
    // also can't match, so the orchestrator returns the sentinel.
    EXPECT_EQ(parse_timestamp("1234abc"), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_timestamp, overflow_returns_sentinel) {
    EXPECT_EQ(parse_timestamp("9999999999999999999999999"),
              UNPARSEABLE_TIMESTAMP_SENTINEL);
}

TEST(parse_timestamp, numeric_epoch_succeeds_via_first_branch) {
    EXPECT_EQ(parse_timestamp("1448931600"), int64_t{1448931600});
}

TEST(parse_timestamp, formatted_string_succeeds_via_cascade) {
    // The cascade case the helper split must preserve: epoch parser
    // returns the sentinel (consumed only "2015"), parse_timestamp
    // must then call the formatted parser, which succeeds.
    EXPECT_EQ(parse_timestamp("2015-12-01 01:00:00"), int64_t{1448931600});
}

// =========================================================================
// UNPARSEABLE_TIMESTAMP_SENTINEL — pin the value and the contract.
// =========================================================================

TEST(unparseable_timestamp_sentinel, equals_int64_min) {
    // The sentinel is INT64_MIN; producers stamp this on records
    // when `parse_timestamp` fails and the save still needs to
    // land. Pinning the literal so any future change to the
    // constant surfaces as a test failure rather than silent
    // wire-format drift between producer and consumer.
    EXPECT_EQ(UNPARSEABLE_TIMESTAMP_SENTINEL, std::numeric_limits<int64_t>::min());
}

TEST(unparseable_timestamp_sentinel, distinct_from_any_parsed_value) {
    // The sentinel must not collide with anything `parse_timestamp`
    // can return for a legitimate input. INT64_MIN as Unix epoch
    // sits ~9.2e18 seconds before 1970 — no real simulated moment
    // (including paleo-hydrology runs) approaches it.
    EXPECT_NE(parse_timestamp("0"), UNPARSEABLE_TIMESTAMP_SENTINEL);
    EXPECT_NE(parse_timestamp("-3786825600"), UNPARSEABLE_TIMESTAMP_SENTINEL); // 1850
    EXPECT_NE(parse_timestamp("1448931600"), UNPARSEABLE_TIMESTAMP_SENTINEL);  // 2015
    EXPECT_NE(parse_timestamp("2015-12-01 01:00:00"), UNPARSEABLE_TIMESTAMP_SENTINEL);
}

// =========================================================================
// Pinning test against ngen's Simulation_Time format.
// =========================================================================

TEST(parse_timestamp, matches_simulation_time_format) {
    // If this fails, ngen's `Simulation_Time::get_timestamp` has
    // changed its strftime format. Update TIMESTAMP_STRPTIME_FORMAT
    // in record.hpp to match, or producer and consumer will silently
    // disagree on `simulation_timestamp` encoding.
    //
    // The expected string MUST be byte-for-byte what
    // `Simulation_Time::get_timestamp` produces for a known epoch.
    // 1448931600 = 2015-12-01T01:00:00Z.
    const int64_t got = parse_timestamp("2015-12-01 01:00:00");
    EXPECT_NE(got, UNPARSEABLE_TIMESTAMP_SENTINEL)
        << "Format mismatch with Simulation_Time::get_timestamp. "
        << "TIMESTAMP_STRPTIME_FORMAT is currently '"
        << TIMESTAMP_STRPTIME_FORMAT << "'.";
    EXPECT_EQ(got, int64_t{1448931600});
}
