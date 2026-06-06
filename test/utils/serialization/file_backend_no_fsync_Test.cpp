/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Exercises the FileBackend's non-POSIX code path on a POSIX host. This
TU is linked into a separate test executable whose build defines
`NGEN_HAVE_POSIX_FSYNC=0`, which forces the same compile of
`file_backend.cpp` that a non-POSIX system would produce. The test
then drives the public `FileBackend::create(...)` factory and
verifies that `writer(Durability::strict)` returns the documented
error directing the caller to use `Durability::relaxed` instead.

Why a separate test executable? The macro is consumed at compile
time inside `file_backend.cpp`. The production `ngen_bmi_protocols`
library compiles that TU with the macro set to 1 (on this host); we
cannot mix the two compilations into one linked artifact (ODR). The
no-fsync build is therefore a standalone executable that compiles
`file_backend.cpp` itself rather than linking the production
library.
*/
#include "gtest/gtest.h"

#include "utilities/bmi/file_backend.hpp"
#include "utilities/serialization/record.hpp"
#include "utilities/serialization/record_backend.hpp"

#include <chrono>
#include <cstdio>
#include <string>

using ngen::serialization::BackendError;
using ngen::serialization::Durability;
using ngen::serialization::FileBackend;

namespace {
std::string temp_path() {
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string dir  = ::testing::TempDir();
    if (!dir.empty() && dir.back() != '/') dir.push_back('/');
    return dir + "ngen_filebackend_nofsync_" + info->name() + ".bin";
}

} // namespace

TEST(FileBackendNoFsync, strict_durability_errors_with_actionable_message) {
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be_result = FileBackend::create(path);
    ASSERT_TRUE(be_result.has_value()) << be_result.error().message;
    auto be = std::move(be_result.value());
    auto w  = be->writer(std::chrono::system_clock::now(), Durability::strict);

    ASSERT_FALSE(w.has_value());
    EXPECT_EQ(w.error().kind, BackendError::Kind::IOError);
    // Message should identify strict-not-supported AND point the
    // caller at Durability::relaxed as the recovery path.
    EXPECT_NE(w.error().message.find("Durability::strict"), std::string::npos)
        << "actual: " << w.error().message;
    EXPECT_NE(w.error().message.find("Durability::relaxed"), std::string::npos)
        << "actual: " << w.error().message;

    std::remove(path.c_str());
}

TEST(FileBackendNoFsync, relaxed_durability_still_works) {
    // The non-POSIX build is only restrictive about strict; relaxed
    // must remain fully functional (it's the only available mode on
    // that platform).
    const std::string path = temp_path();
    std::remove(path.c_str());

    auto be_result = FileBackend::create(path);
    ASSERT_TRUE(be_result.has_value()) << be_result.error().message;
    auto be = std::move(be_result.value());
    {
        auto w = be->writer(std::chrono::system_clock::now(), Durability::relaxed);
        ASSERT_TRUE(w.has_value()) << w.error().message;
        ngen::serialization::Record rec{"cat-1", 0, 0, std::vector<char>{'X'}, 0};
        ASSERT_TRUE(w.value()->write(rec).has_value());
        ASSERT_TRUE(w.value()->commit().has_value());
    }
    auto r = be->reader();
    ASSERT_TRUE(r.has_value());
    auto rec = r.value()->find_latest("cat-1");
    ASSERT_TRUE(rec.has_value()) << rec.error().message;
    EXPECT_EQ(rec.value().payload, std::vector<char>{'X'});

    std::remove(path.c_str());
}
