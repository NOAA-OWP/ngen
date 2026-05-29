/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Tests for `ngen::serialization::id_predicates.hpp`. Each stock helper
and combinator is exercised against the id-composition conventions
the library promises to support.
*/
#include "gtest/gtest.h"

#include "utilities/serialization/id_predicates.hpp"

using namespace ngen::serialization;

TEST(IdPredicates, exact_id_matches_identical_string) {
    auto p = exact_id("cat-1:cfe");
    EXPECT_TRUE (p("cat-1:cfe"));
    EXPECT_FALSE(p("cat-1"));
    EXPECT_FALSE(p("cat-1:cfe:multi"));
    EXPECT_FALSE(p(""));
}

TEST(IdPredicates, primary_prefix_matches_first_segment) {
    auto p = primary_prefix("cat-1");
    // Plain colon split:
    EXPECT_TRUE (p("cat-1:cfe"));
    EXPECT_TRUE (p("cat-1:cfe:multi"));
    // Dot split (used by Bmi_Multi_Formulation submodule indices):
    EXPECT_TRUE (p("cat-1.0:cfe:multi"));
    // No delimiter — full string is the primary:
    EXPECT_TRUE (p("cat-1"));
    // Different first segment:
    EXPECT_FALSE(p("cat-2:cfe"));
    EXPECT_FALSE(p("cat-1-extra:cfe"));  // not a partial match
}

TEST(IdPredicates, contains_segment_matches_anywhere_in_chain) {
    auto p = contains_segment("key_1");
    EXPECT_TRUE (p("cat-1:some_model:key_1:hash:key_2"));
    EXPECT_TRUE (p("key_1:cat-1"));
    EXPECT_TRUE (p("cat-1:key_1"));
    EXPECT_TRUE (p("key_1"));  // single segment
    // Substring-not-token must NOT match:
    EXPECT_FALSE(p("cat-1:key_10:other"));
    EXPECT_FALSE(p("cat-1:prefix_key_1:other"));
    // Different segment:
    EXPECT_FALSE(p("cat-1:cfe:multi"));
}

TEST(IdPredicates, at_position_matches_specific_segment_index) {
    auto p = at_position(2, "key_1");
    // Segments: [cat-1, some_model, key_1, hash, key_2] → index 2 is "key_1"
    EXPECT_TRUE (p("cat-1:some_model:key_1:hash:key_2"));
    // Same value but wrong index:
    EXPECT_FALSE(p("key_1:some_model:other:hash:key_2"));  // at index 0
    // Id too short:
    EXPECT_FALSE(p("cat-1:some_model"));
    EXPECT_FALSE(p("cat-1"));
    EXPECT_FALSE(p(""));
}

TEST(IdPredicates, starts_with_matches_substring_prefix) {
    auto p = starts_with("rank-0:");
    EXPECT_TRUE (p("rank-0:cat-1:cfe"));
    EXPECT_TRUE (p("rank-0:"));  // exact-prefix length match
    EXPECT_FALSE(p("rank-1:cat-1:cfe"));
    EXPECT_FALSE(p("rank-0"));   // shorter than prefix
    EXPECT_FALSE(p(""));
}

TEST(IdPredicates, all_of_requires_every_predicate_true) {
    auto p = all_of({contains_segment("key_1"), contains_segment("key_2")});
    EXPECT_TRUE (p("cat-1:key_1:key_2"));
    EXPECT_TRUE (p("key_2:key_1"));            // order-independent
    EXPECT_FALSE(p("cat-1:key_1:other"));      // missing key_2
    EXPECT_FALSE(p("cat-1:other:key_2"));      // missing key_1
    EXPECT_FALSE(p(""));                       // missing both
}

TEST(IdPredicates, any_of_requires_one_predicate_true) {
    auto p = any_of({primary_prefix("cat-1"), primary_prefix("cat-2")});
    EXPECT_TRUE (p("cat-1:cfe"));
    EXPECT_TRUE (p("cat-2:cfe"));
    EXPECT_FALSE(p("cat-3:cfe"));
    EXPECT_FALSE(p(""));
}

TEST(IdPredicates, not_fn_inverts_a_predicate) {
    auto p = not_fn(primary_prefix("cat-1"));
    EXPECT_FALSE(p("cat-1:cfe"));
    EXPECT_TRUE (p("cat-2:cfe"));
    EXPECT_TRUE (p(""));
}

TEST(IdPredicates, custom_closure_works_as_application_seam) {
    // An application-defined predicate the library has no built-in for:
    // match any id whose third segment is a four-digit string.
    IdPredicate p = [](const std::string& id) {
        std::size_t start = 0;
        for (std::size_t i = 0; i < 2; ++i) {
            auto d = id.find_first_of(":.", start);
            if (d == std::string::npos) return false;
            start = d + 1;
        }
        auto end = id.find_first_of(":.", start);
        if (end == std::string::npos) end = id.size();
        if (end - start != 4) return false;
        for (auto i = start; i < end; ++i) {
            if (id[i] < '0' || id[i] > '9') return false;
        }
        return true;
    };
    EXPECT_TRUE (p("cat-1:cfe:1234"));
    EXPECT_TRUE (p("cat-1:cfe:0001:more"));
    EXPECT_FALSE(p("cat-1:cfe:abcd"));
    EXPECT_FALSE(p("cat-1:cfe:12345"));  // wrong length
    EXPECT_FALSE(p("cat-1:cfe"));        // too short
}

TEST(IdPredicates, default_constructed_predicate_is_empty) {
    // The abstract `RecordBackend::reader(IdPredicate)` accepts a
    // default-constructed `std::function` to mean "no scope." We
    // don't expect anyone to *invoke* an empty predicate; that
    // would throw. But the type should be default-constructible
    // and convert to bool meaningfully.
    IdPredicate p;
    EXPECT_FALSE(static_cast<bool>(p));
}
