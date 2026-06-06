/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Tests for the Reader side of `RecordBackend`: the `reader()` factory,
the `Reader` interface (`find_*`, `close()`), and the `with_reader`
convenience wrapper. Covers both the default no-op `close()` path and
the override path that surfaces fallible read-side teardown.
*/
#include "gtest/gtest.h"

#include "mocks.hpp"
#include "utilities/serialization/id_predicates.hpp"
#include "utilities/serialization/record_backend.hpp"

using namespace ngen::serialization;
using ngen::serialization::test::FailingBackend;
using ngen::serialization::test::NoopBackend;
using ngen::serialization::test::TrackingBackend;
using Reader = RecordBackend::Reader;
using Writer = RecordBackend::Writer;

// ---------------------------------------------------------------------
// reader() factory
// ---------------------------------------------------------------------

TEST(RecordBackend_reader, factory_returns_a_reader_on_happy_path) {
    TrackingBackend be;
    auto r = be.reader(primary_prefix("cat-1"));
    ASSERT_TRUE(r.has_value());
    EXPECT_NE(r.value().get(), nullptr);
    EXPECT_EQ(be.readers_constructed, 1);
}

TEST(RecordBackend_reader, multiple_readers_per_backend_are_allowed) {
    TrackingBackend be;
    auto r1 = be.reader(primary_prefix("cat-1"));
    auto r2 = be.reader(primary_prefix("cat-2"));
    auto r3 = be.reader(IdPredicate{}); // empty / unscoped
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    ASSERT_TRUE(r3.has_value());
    EXPECT_EQ(be.readers_constructed, 3);
}

TEST(RecordBackend_reader, factory_failure_propagates_on_error_arm) {
    FailingBackend be;
    be.reader_factory_fails = true;
    auto r                  = be.reader(IdPredicate{});
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().kind, BackendError::Kind::IOError);
}

// ---------------------------------------------------------------------
// find_* error categories
// ---------------------------------------------------------------------

TEST(RecordBackend_reader, find_returns_record_on_happy_path) {
    NoopBackend be;
    auto r = be.reader(IdPredicate{});
    ASSERT_TRUE(r.has_value());

    EXPECT_TRUE(r.value()->find_latest("x").has_value());
    EXPECT_TRUE(r.value()->find_at_step("x", 0).has_value());
    EXPECT_TRUE(r.value()->find_at_simulation_timestamp("x", 0).has_value());
}

TEST(RecordBackend_reader, find_returns_NotFound_via_error_arm) {
    FailingBackend be;
    be.find_returns_not_found = true;
    auto r                    = be.reader(IdPredicate{});
    ASSERT_TRUE(r.has_value());

    auto rec = r.value()->find_latest("missing");
    ASSERT_FALSE(rec.has_value());
    EXPECT_EQ(rec.error().kind, BackendError::Kind::NotFound);
}

TEST(RecordBackend_reader, find_returns_Corrupted_via_error_arm) {
    FailingBackend be;
    be.find_returns_corrupted = true;
    auto r                    = be.reader(IdPredicate{});
    ASSERT_TRUE(r.has_value());

    auto rec = r.value()->find_at_step("garbage", 5);
    ASSERT_FALSE(rec.has_value());
    EXPECT_EQ(rec.error().kind, BackendError::Kind::Corrupted);
}

TEST(RecordBackend_reader, NotFound_and_Corrupted_are_distinguishable_by_kind) {
    // The whole point of categorized errors: callers branch on `kind`
    // without parsing the message.
    FailingBackend not_found_be;
    not_found_be.find_returns_not_found = true;
    FailingBackend corrupted_be;
    corrupted_be.find_returns_corrupted = true;

    auto r_nf = not_found_be.reader(IdPredicate{});
    auto r_c  = corrupted_be.reader(IdPredicate{});
    ASSERT_TRUE(r_nf.has_value());
    ASSERT_TRUE(r_c.has_value());

    auto rec_nf = r_nf.value()->find_latest("x");
    auto rec_c  = r_c.value()->find_latest("x");
    ASSERT_FALSE(rec_nf.has_value());
    ASSERT_FALSE(rec_c.has_value());
    EXPECT_NE(rec_nf.error().kind, rec_c.error().kind);
}

// ---------------------------------------------------------------------
// Reader::close — default no-op + override paths
// ---------------------------------------------------------------------

TEST(RecordBackend_reader, default_close_is_a_noop_success) {
    NoopBackend be;
    auto r = be.reader(IdPredicate{});
    ASSERT_TRUE(r.has_value());

    EXPECT_TRUE(r.value()->close().has_value());
    // Calling close() twice is fine on the no-op default — neither
    // call sees the previous one.
    EXPECT_TRUE(r.value()->close().has_value());
}

TEST(RecordBackend_reader, overridden_close_can_succeed) {
    TrackingBackend be;
    auto r = be.reader(IdPredicate{});
    ASSERT_TRUE(r.has_value());

    EXPECT_TRUE(r.value()->close().has_value());
    EXPECT_EQ(be.reader_closes_called, 1);
}

TEST(RecordBackend_reader, overridden_close_can_fail) {
    FailingBackend be;
    be.reader_close_fails = true;
    auto r                = be.reader(IdPredicate{});
    ASSERT_TRUE(r.has_value());

    auto rc = r.value()->close();
    ASSERT_FALSE(rc.has_value());
    EXPECT_EQ(rc.error().kind, BackendError::Kind::IOError);
    EXPECT_NE(rc.error().message.find("close"), std::string::npos);
}

// ---------------------------------------------------------------------
// with_reader convenience wrapper
// ---------------------------------------------------------------------

TEST(RecordBackend_with_reader, calls_close_on_success_path) {
    TrackingBackend be;
    auto rc = be.with_reader(IdPredicate{}, [](Reader& r) { (void)r.find_latest("x"); });
    ASSERT_TRUE(rc.has_value());
    EXPECT_EQ(be.reader_closes_called, 1);
    EXPECT_EQ(be.readers_destroyed, 1);
}

TEST(RecordBackend_with_reader, default_close_makes_wrapper_inexpensive) {
    NoopBackend be;
    // With the default no-op close, with_reader works seamlessly even
    // for backends that haven't overridden close().
    auto rc = be.with_reader(IdPredicate{}, [](Reader& r) { (void)r.find_latest("x"); });
    EXPECT_TRUE(rc.has_value());
}

TEST(RecordBackend_with_reader, factory_failure_propagates_without_calling_fn) {
    FailingBackend be;
    be.reader_factory_fails = true;

    int closure_calls = 0;
    auto rc           = be.with_reader(IdPredicate{}, [&](Reader&) { ++closure_calls; });
    EXPECT_FALSE(rc.has_value());
    EXPECT_EQ(closure_calls, 0);
}

TEST(RecordBackend_with_reader, close_failure_propagates_after_fn_ran) {
    FailingBackend be;
    be.reader_close_fails = true;

    int closure_calls = 0;
    auto rc           = be.with_reader(IdPredicate{}, [&](Reader& r) {
        ++closure_calls;
        (void)r.find_latest("x");
    });
    EXPECT_FALSE(rc.has_value());
    EXPECT_EQ(rc.error().kind, BackendError::Kind::IOError);
    EXPECT_EQ(closure_calls, 1); // fn ran before close failed
}

TEST(RecordBackend_with_reader, exception_from_fn_unwinds_through_destructor) {
    TrackingBackend be;
    EXPECT_THROW(
        { (void)be.with_reader(IdPredicate{}, [](Reader&) { throw std::runtime_error("boom"); }); },
        std::runtime_error
    );

    // Reader destroyed; close NOT called (we threw before it).
    EXPECT_EQ(be.readers_destroyed, 1);
    EXPECT_EQ(be.reader_closes_called, 0);

    // Backend still usable:
    auto rc = be.with_reader(IdPredicate{}, [](Reader&) {});
    EXPECT_TRUE(rc.has_value());
}
