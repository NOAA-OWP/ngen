/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Tests for the Writer side of `RecordBackend`: the `writer()` factory,
the `Writer` interface, the single-in-flight rule, and the
`with_writer` convenience wrapper.
*/
#include "gtest/gtest.h"

#include "mocks.hpp"
#include "utilities/serialization/record_backend.hpp"

#include <chrono>

using namespace ngen::serialization;
using ngen::serialization::test::FailingBackend;
using ngen::serialization::test::TrackingBackend;
using Reader = RecordBackend::Reader;
using Writer = RecordBackend::Writer;

namespace {
auto now() {
    return std::chrono::system_clock::now();
}
} // namespace

// ---------------------------------------------------------------------
// writer() factory
// ---------------------------------------------------------------------

TEST(RecordBackend_writer, factory_returns_a_writer_on_happy_path) {
    TrackingBackend be;
    auto w = be.writer(now(), Durability::relaxed);
    ASSERT_TRUE(w.has_value());
    EXPECT_NE(w.value().get(), nullptr);
    EXPECT_EQ(be.writers_constructed, 1);
}

TEST(RecordBackend_writer, single_in_flight_rule_rejects_second_writer) {
    TrackingBackend be;
    auto first = be.writer(now(), Durability::relaxed);
    ASSERT_TRUE(first.has_value());

    auto second = be.writer(now(), Durability::relaxed);
    EXPECT_FALSE(second.has_value());
    EXPECT_EQ(second.error().kind, BackendError::Kind::IOError);
    EXPECT_NE(second.error().message.find("single-in-flight"), std::string::npos);

    // Only one Writer ever constructed:
    EXPECT_EQ(be.writers_constructed, 1);
}

TEST(RecordBackend_writer, dropping_writer_clears_in_flight_flag) {
    TrackingBackend be;
    {
        auto w = be.writer(now(), Durability::relaxed);
        ASSERT_TRUE(w.has_value());
    } // Writer destroyed here
    EXPECT_EQ(be.writers_destroyed, 1);
    EXPECT_FALSE(be.writer_in_flight);

    // Next writer can proceed:
    auto next = be.writer(now(), Durability::relaxed);
    EXPECT_TRUE(next.has_value());
}

TEST(RecordBackend_writer, factory_failure_propagates_on_error_arm) {
    FailingBackend be;
    be.writer_factory_fails = true;
    auto w                  = be.writer(now(), Durability::relaxed);
    ASSERT_FALSE(w.has_value());
    EXPECT_EQ(w.error().kind, BackendError::Kind::IOError);
}

// ---------------------------------------------------------------------
// Writer::write / commit
// ---------------------------------------------------------------------

TEST(RecordBackend_writer, write_and_commit_succeed) {
    TrackingBackend be;
    auto w = be.writer(now(), Durability::strict);
    ASSERT_TRUE(w.has_value());

    EXPECT_TRUE(w.value()->write(RecordView{}).has_value());
    EXPECT_TRUE(w.value()->write(RecordView{}).has_value());
    EXPECT_EQ(be.writes_called, 2);

    EXPECT_TRUE(w.value()->commit().has_value());
    EXPECT_EQ(be.writer_commits_called, 1);
}

TEST(RecordBackend_writer, write_failure_surfaces_on_error_arm) {
    FailingBackend be;
    be.write_fails = true;
    auto w         = be.writer(now(), Durability::relaxed);
    ASSERT_TRUE(w.has_value());

    auto r = w.value()->write(RecordView{});
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().kind, BackendError::Kind::IOError);
    EXPECT_EQ(r.error().message, "forced write failure");
}

TEST(RecordBackend_writer, commit_failure_surfaces_on_error_arm) {
    FailingBackend be;
    be.writer_commit_fails = true;
    auto w                 = be.writer(now(), Durability::strict);
    ASSERT_TRUE(w.has_value());

    auto r = w.value()->commit();
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().kind, BackendError::Kind::IOError);
}

// ---------------------------------------------------------------------
// with_writer convenience wrapper
// ---------------------------------------------------------------------

TEST(RecordBackend_with_writer, calls_commit_on_success_path) {
    TrackingBackend be;
    auto rc = be.with_writer(now(), Durability::relaxed, [](Writer& w) {
        (void)w.write(RecordView{});
        (void)w.write(RecordView{});
    });
    ASSERT_TRUE(rc.has_value());
    EXPECT_EQ(be.writes_called, 2);
    EXPECT_EQ(be.writer_commits_called, 1);
    // Writer destroyed before with_writer returned:
    EXPECT_EQ(be.writers_destroyed, 1);
    EXPECT_FALSE(be.writer_in_flight);
}

TEST(RecordBackend_with_writer, factory_failure_propagates_without_calling_fn) {
    FailingBackend be;
    be.writer_factory_fails = true;

    int closure_calls = 0;
    auto rc = be.with_writer(now(), Durability::relaxed, [&](Writer&) { ++closure_calls; });
    EXPECT_FALSE(rc.has_value());
    EXPECT_EQ(rc.error().kind, BackendError::Kind::IOError);
    EXPECT_EQ(closure_calls, 0);
}

TEST(RecordBackend_with_writer, commit_failure_propagates_after_fn_ran) {
    FailingBackend be;
    be.writer_commit_fails = true;

    int closure_calls = 0;
    auto rc           = be.with_writer(now(), Durability::strict, [&](Writer& w) {
        ++closure_calls;
        (void)w.write(RecordView{});
    });
    EXPECT_FALSE(rc.has_value());
    EXPECT_EQ(rc.error().kind, BackendError::Kind::IOError);
    EXPECT_EQ(closure_calls, 1); // fn ran before commit failed
}

TEST(RecordBackend_with_writer, sequential_calls_do_not_trip_single_in_flight) {
    TrackingBackend be;
    for (int i = 0; i < 5; ++i) {
        auto rc =
            be.with_writer(now(), Durability::relaxed, [](Writer& w) { (void)w.write(RecordView{}); });
        ASSERT_TRUE(rc.has_value()) << "iteration " << i;
    }
    EXPECT_EQ(be.writers_constructed, 5);
    EXPECT_EQ(be.writers_destroyed, 5);
    EXPECT_EQ(be.writer_commits_called, 5);
    EXPECT_EQ(be.writes_called, 5);
}

TEST(RecordBackend_with_writer, exception_from_fn_unwinds_with_destructor_cleanup) {
    TrackingBackend be;
    EXPECT_THROW(
        {
            (void)be.with_writer(now(), Durability::relaxed, [](Writer&) {
                throw std::runtime_error("boom");
            });
        },
        std::runtime_error
    );

    // Writer destructor must have run (in-flight flag cleared).
    EXPECT_EQ(be.writers_destroyed, 1);
    EXPECT_FALSE(be.writer_in_flight);
    // commit() should NOT have been called — the closure threw.
    EXPECT_EQ(be.writer_commits_called, 0);

    // Next writer can proceed:
    auto rc = be.with_writer(now(), Durability::relaxed, [](Writer&) {});
    EXPECT_TRUE(rc.has_value());
}
