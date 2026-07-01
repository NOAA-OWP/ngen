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
Tests for the realization-layer path-template token resolver. These
are intentionally narrow: the resolver is a pure string operation
and its only runtime dependencies (pid / hostname / MPI rank) are
best validated by checking structural properties, not exact values.
*/
#include "gtest/gtest.h"
#include "path_tokens.hpp"

#include <regex>
#include <string>

using utilities::resolve_path_tokens;

TEST(PathTokensTest, passthrough_when_no_tokens) {
    EXPECT_EQ(resolve_path_tokens("/a/b/c.bin"), "/a/b/c.bin");
    EXPECT_EQ(resolve_path_tokens(""), "");
}

TEST(PathTokensTest, rank_resolves_to_digits) {
    const std::string out = resolve_path_tokens("ckpt.rank_{{rank}}.bin");
    // Outside of an initialized MPI context the resolver returns "0"; in
    // either case the replacement is always a run of decimal digits.
    EXPECT_TRUE(std::regex_match(out, std::regex(R"(ckpt\.rank_\d+\.bin)")))
        << "got: " << out;
}

TEST(PathTokensTest, pid_resolves_to_digits) {
    const std::string out = resolve_path_tokens("p{{pid}}.bin");
    EXPECT_TRUE(std::regex_match(out, std::regex(R"(p\d+\.bin)")))
        << "got: " << out;
}

TEST(PathTokensTest, host_resolves_nonempty_or_empty_label) {
    // gethostname may legally fail in a container; the resolver returns
    // an empty string in that case, which would produce "h.bin". Assert
    // one of the two shapes.
    const std::string out = resolve_path_tokens("h{{host}}.bin");
    EXPECT_TRUE(
        std::regex_match(out, std::regex(R"(h[A-Za-z0-9\-_]*\.bin)"))
    ) << "got: " << out;
}

TEST(PathTokensTest, date_resolves_to_yyyymmdd) {
    const std::string out = resolve_path_tokens("run_{{date}}/x.bin");
    EXPECT_TRUE(std::regex_match(out, std::regex(R"(run_\d{8}/x\.bin)")))
        << "got: " << out;
}

TEST(PathTokensTest, multiple_tokens_compose) {
    const std::string out = resolve_path_tokens("d{{date}}/r{{rank}}.bin");
    EXPECT_TRUE(std::regex_match(out, std::regex(R"(d\d{8}/r\d+\.bin)")))
        << "got: " << out;
}

TEST(PathTokensTest, unknown_tokens_pass_through_untouched) {
    // Preserves unknown tokens so typos don't silently vanish. Matches
    // the header's documented contract.
    EXPECT_EQ(resolve_path_tokens("{{bogus}}/x"), "{{bogus}}/x");
    EXPECT_EQ(resolve_path_tokens("{{RANK}}"), "{{RANK}}");  // case-sensitive
}

TEST(PathTokensTest, single_brace_is_literal) {
    // A lone `{` or `{{...}` doesn't match; brace pairs only.
    EXPECT_EQ(resolve_path_tokens("{not_a_token}/{{rank"), "{not_a_token}/{{rank");
}

TEST(PathTokensTest, adjacent_tokens) {
    const std::string out = resolve_path_tokens("{{rank}}{{pid}}");
    EXPECT_TRUE(std::regex_match(out, std::regex(R"(\d+\d+)")))
        << "got: " << out;
}
