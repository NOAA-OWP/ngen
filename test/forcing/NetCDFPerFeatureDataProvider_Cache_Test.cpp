#include <NGenConfig.h>

#if NGEN_WITH_NETCDF

#include "gtest/gtest.h"
#include "NetCDFPerFeatureDataProvider.hpp"

#include <algorithm>
#include <atomic>
#include <barrier>
#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

// Concurrency tests for NetCDFPerFeatureDataProvider::cache_slot and
// NetCDFPerFeatureDataProvider::shared_cache.
//
// When built with GCC or Clang, the test target compiles the cache
// implementation itself with -fsanitize=thread, so these tests are
// meant to be judged by ThreadSanitizer's verdict as much as by their
// assertions. To keep that verdict meaningful, the tests take care not
// to create happens-before edges between a filler and its readers other
// than through the slot: flags passed between those threads are relaxed,
// and buffers are created by the filling thread after it starts.

namespace {

using Provider = data_access::NetCDFPerFeatureDataProvider;
using cache_slot = Provider::cache_slot;
using shared_cache = Provider::shared_cache;
using buffer_type = Provider::cache_buffer_type;
using key_type = Provider::cache_key_type;

constexpr std::size_t buffer_size = 1024;

unsigned test_thread_count()
{
    return std::clamp(std::thread::hardware_concurrency(), 4u, 16u);
}

buffer_type make_buffer(double seed)
{
    auto buffer = std::make_shared<std::vector<double>>(buffer_size);
    for (std::size_t i = 0; i < buffer_size; ++i)
        (*buffer)[i] = seed + i;
    return buffer;
}

// True if every element written by make_buffer(seed) is visible through buffer
bool buffer_holds(buffer_type const& buffer, double seed)
{
    if (!buffer || buffer->size() != buffer_size)
        return false;
    for (std::size_t i = 0; i < buffer_size; ++i)
        if ((*buffer)[i] != seed + i)
            return false;
    return true;
}

// Run body(thread_index) on n_threads std::threads and join them all
void run_threads(unsigned n_threads, std::function<void(unsigned)> const& body)
{
    std::vector<std::thread> threads;
    threads.reserve(n_threads);
    for (unsigned t = 0; t < n_threads; ++t)
        threads.emplace_back(body, t);
    for (auto& thread : threads)
        thread.join();
}

// Spin for a pseudo-random handful of yields, to shuffle the order in
// which threads reach the operations under test
void jitter(std::minstd_rand& rng)
{
    for (int n = std::uniform_int_distribution<int>(0, 64)(rng); n > 0; --n)
        std::this_thread::yield();
}

} // namespace

// ---------------------------------------------------------------- cache_slot

TEST(CacheSlotTest, GetAfterFillReturnsFilledBuffer)
{
    for (bool immediate_use : {false, true}) {
        cache_slot slot;
        auto buffer = make_buffer(1.0);
        slot.fill(buffer, immediate_use);

        // Repeated get() is idempotent, whichever state fill() left
        EXPECT_EQ(slot.get(), buffer) << "immediate_use = " << immediate_use;
        EXPECT_EQ(slot.get(), buffer) << "immediate_use = " << immediate_use;
    }
}

TEST(CacheSlotTest, SlotSharesOwnershipOfBuffer)
{
    std::weak_ptr<std::vector<double>> weak;
    {
        cache_slot slot;
        {
            auto buffer = make_buffer(2.0);
            weak = buffer;
            slot.fill(std::move(buffer), false);
        }
        // The filler's reference is gone; the slot keeps the buffer alive
        ASSERT_FALSE(weak.expired());

        auto got = slot.get();
        EXPECT_EQ(got.use_count(), 2); // slot + got
        EXPECT_TRUE(buffer_holds(got, 2.0));
    }
    // Destroying the slot and the reader's copy releases the buffer
    EXPECT_TRUE(weak.expired());
}

TEST(CacheSlotTest, GetWaitsForFill)
{
    const unsigned n_waiters = test_thread_count() - 1;
    cache_slot slot;

    std::atomic<unsigned> arrived{0};
    std::atomic<bool> fill_begun{false};
    std::vector<buffer_type> got(n_waiters);
    std::vector<char> saw_fill_begun(n_waiters, false);
    buffer_type filled;

    run_threads(n_waiters + 1, [&](unsigned t) {
        if (t == n_waiters) {
            // Filler: let the waiters get well into get() first
            while (arrived.load(std::memory_order_relaxed) < n_waiters)
                std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));

            filled = make_buffer(3.0);
            fill_begun.store(true, std::memory_order_relaxed);
            slot.fill(filled, true);
            return;
        }

        arrived.fetch_add(1, std::memory_order_relaxed);
        got[t] = slot.get();
        // fill_begun was stored before fill(), so a get() that really
        // waited for fill() must observe it
        saw_fill_begun[t] = fill_begun.load(std::memory_order_relaxed);
    });

    for (unsigned t = 0; t < n_waiters; ++t) {
        EXPECT_TRUE(saw_fill_begun[t]) << "waiter " << t << " returned from get() before fill()";
        EXPECT_EQ(got[t], filled) << "waiter " << t;
        EXPECT_TRUE(buffer_holds(got[t], 3.0)) << "waiter " << t;
    }
}

TEST(CacheSlotTest, ConcurrentFillAndGetPublishBufferContents)
{
    // Each round uses a fresh slot, with the filling role rotating
    // among threads. Jitter mixes rounds where readers block in get()
    // with rounds where they arrive after fill(), and alternating
    // immediate_use covers both FILLED and HOT publication.
    constexpr unsigned n_rounds = 200;
    const unsigned n_threads = test_thread_count();

    auto slots = std::make_unique<cache_slot[]>(n_rounds);
    std::vector<char> ok(n_rounds * n_threads, false);
    std::barrier round_start(n_threads);

    run_threads(n_threads, [&](unsigned t) {
        std::minstd_rand rng(t + 1);
        for (unsigned r = 0; r < n_rounds; ++r) {
            round_start.arrive_and_wait();
            jitter(rng);

            const double seed = r * 10.0;
            buffer_type got;
            if (r % n_threads == t) {
                got = make_buffer(seed);
                slots[r].fill(got, r % 2 == 0);
            } else {
                got = slots[r].get();
            }
            ok[r * n_threads + t] = buffer_holds(got, seed);
        }
    });

    for (unsigned r = 0; r < n_rounds; ++r)
        for (unsigned t = 0; t < n_threads; ++t)
            EXPECT_TRUE(ok[r * n_threads + t]) << "round " << r << ", thread " << t;
}

TEST(CacheSlotTest, ReaderAfterFirstReaderOfColdFillSeesContents)
{
    // A fill() with immediate_use = false leaves the slot FILLED, and
    // the first get() moves it to HOT. A later reader that observes HOT
    // must still see everything the filler wrote. The second reader is
    // ordered after the first by a relaxed flag only, so it has no
    // happens-before edge with either other thread except via the slot.
    //
    // NB: ThreadSanitizer cannot flag a broken release sequence here,
    // since it treats a relaxed store as preserving the previous release
    // store's synchronization. This test pins the behavior; see the
    // review notes for the memory model argument.
    for (int round = 0; round < 50; ++round) {
        cache_slot slot;
        std::atomic<bool> first_done{false};
        buffer_type second_got;

        run_threads(3, [&](unsigned t) {
            switch (t) {
            case 0:
                slot.fill(make_buffer(round), false);
                break;
            case 1:
                slot.get();
                first_done.store(true, std::memory_order_relaxed);
                break;
            case 2:
                while (!first_done.load(std::memory_order_relaxed))
                    std::this_thread::yield();
                second_got = slot.get();
                break;
            }
        });

        EXPECT_TRUE(buffer_holds(second_got, round)) << "round " << round;
    }
}

// -------------------------------------------------------------- shared_cache

TEST(SharedCacheTest, FirstLookupInsertsLaterLookupsFind)
{
    shared_cache cache;
    key_type key{0, "APCP_surface"};

    auto [first_slot, first_inserted] = cache.find_or_insert(key, false);
    auto [second_slot, second_inserted] = cache.find_or_insert(key, false);

    EXPECT_TRUE(first_inserted);
    EXPECT_FALSE(second_inserted);
    EXPECT_EQ(&first_slot, &second_slot);
}

TEST(SharedCacheTest, SlotReferencesSurviveNonEvictingInserts)
{
    shared_cache cache;
    key_type key{24, "T2D_2maboveground"};
    cache_slot* original = &cache.find_or_insert(key, false).first;

    // Rebalancing std::map under many inserts must not move the slot;
    // keys at the same time index never evict it either
    for (int i = 0; i < 1000; ++i) {
        key_type other{(i % 2) ? 24 : 1000 + i, "var" + std::to_string(i)};
        cache.find_or_insert(other, /* evict_older = */ i % 2 == 1);
    }

    auto [found, inserted] = cache.find_or_insert(key, false);
    EXPECT_FALSE(inserted);
    EXPECT_EQ(&found, original);
}

TEST(SharedCacheTest, InsertionEvictsOnlyOlderTimeIndices)
{
    struct eviction_case {
        key_type key;
        bool evict_older;
        bool inserted;
        std::set<key_type> remaining;
    };

    // Every case starts from a cache holding {0,a} {0,b} {24,a} {48,b}
    const eviction_case cases[] = {
        //  key        evict_older  inserted  keys remaining afterwards
          { {24, "a"}, true,        false,    { {0, "a"}, {0, "b"}, {24, "a"}, {48, "b"}            } }
        , { {36, "c"}, false,       true,     { {0, "a"}, {0, "b"}, {24, "a"}, {36, "c"}, {48, "b"} } }
        , { {0,  "c"}, true,        true,     { {0, "a"}, {0, "b"}, {0, "c"}, {24, "a"}, {48, "b"}  } }
        , { {12, "a"}, true,        true,     { {12, "a"}, {24, "a"}, {48, "b"}                     } }
        , { {36, "c"}, true,        true,     { {36, "c"}, {48, "b"}                                } }
        , { {96, "a"}, true,        true,     { {96, "a"}                                           } }
    };

    for (auto const& c : cases) {
        shared_cache cache;
        for (key_type k : {key_type{0, "a"}, key_type{0, "b"}, key_type{24, "a"}, key_type{48, "b"}})
            cache.find_or_insert(k, false);

        key_type key = c.key;
        auto inserted = cache.find_or_insert(key, c.evict_older).second;

        std::set<key_type> remaining;
        for (auto const& entry : cache.cache)
            remaining.insert(entry.first);

        const std::string label = "key {" + std::to_string(key.first) + ", " + key.second
                                + "}, evict_older = " + std::to_string(c.evict_older);
        EXPECT_EQ(inserted, c.inserted) << label;
        EXPECT_EQ(remaining, c.remaining) << label;
    }
}

TEST(SharedCacheTest, EvictedKeyIsInsertedAgain)
{
    // "Exactly one thread returns true" holds per slot lifetime, not per
    // key: once evicted, the next lookup of a key elects a new filler
    shared_cache cache;
    key_type old_key{0, "a"};
    key_type new_key{24, "a"};

    EXPECT_TRUE(cache.find_or_insert(old_key, true).second);
    EXPECT_TRUE(cache.find_or_insert(new_key, true).second);
    EXPECT_TRUE(cache.find_or_insert(old_key, true).second);
}

TEST(SharedCacheTest, ConcurrentLookupsOfOneKeyElectOneInserter)
{
    constexpr int n_rounds = 200;
    const unsigned n_threads = test_thread_count();

    struct result { cache_slot* slot; bool inserted; };
    std::vector<result> results(n_rounds * n_threads);
    shared_cache cache;
    std::barrier round_start(n_threads);

    run_threads(n_threads, [&](unsigned t) {
        for (int r = 0; r < n_rounds; ++r) {
            key_type key{r, "APCP_surface"};
            round_start.arrive_and_wait();
            auto [slot, inserted] = cache.find_or_insert(key, false);
            results[r * n_threads + t] = {&slot, inserted};
        }
    });

    for (int r = 0; r < n_rounds; ++r) {
        auto begin = results.begin() + r * n_threads;
        auto end = begin + n_threads;
        EXPECT_EQ(std::count_if(begin, end, [](result const& x) { return x.inserted; }), 1)
            << "round " << r;
        EXPECT_TRUE(std::all_of(begin, end, [&](result const& x) { return x.slot == begin->slot; }))
            << "round " << r;
    }
}

TEST(SharedCacheTest, ConcurrentLookupsOfDistinctKeysElectOneInserterEach)
{
    constexpr int n_keys = 64;
    const unsigned n_threads = test_thread_count();

    struct result { cache_slot* slot = nullptr; bool inserted = false; };
    std::vector<result> results(n_keys * n_threads);
    shared_cache cache;
    std::barrier start(n_threads);

    run_threads(n_threads, [&](unsigned t) {
        std::vector<int> order(n_keys);
        for (int k = 0; k < n_keys; ++k)
            order[k] = k;
        std::shuffle(order.begin(), order.end(), std::minstd_rand(t + 1));

        start.arrive_and_wait();
        for (int k : order) {
            key_type key{k % 4, "var" + std::to_string(k)};
            auto [slot, inserted] = cache.find_or_insert(key, false);
            results[k * n_threads + t] = {&slot, inserted};
        }
    });

    std::set<cache_slot*> distinct_slots;
    for (int k = 0; k < n_keys; ++k) {
        auto begin = results.begin() + k * n_threads;
        auto end = begin + n_threads;
        EXPECT_EQ(std::count_if(begin, end, [](result const& x) { return x.inserted; }), 1)
            << "key " << k;
        EXPECT_TRUE(std::all_of(begin, end, [&](result const& x) { return x.slot == begin->slot; }))
            << "key " << k;
        distinct_slots.insert(begin->slot);
    }
    EXPECT_EQ(distinct_slots.size(), n_keys);
    EXPECT_EQ(cache.cache.size(), n_keys);
}

TEST(SharedCacheTest, ElectedInserterPublishesToAllFinders)
{
    // Mirrors get_value(): threads advance through time in lockstep,
    // looking up one page per variable, with evicting inserts. The
    // inserter fills its slot, while everyone else waits on get().
    //
    // The barrier at the end of each round is load-bearing: without it,
    // an inserter of round r+1 could evict a round r slot that a slower
    // thread still holds (see DISABLED_EvictionWhileAnotherThreadHoldsSlot).
    constexpr int n_rounds = 100;
    constexpr int page_size = 24;
    const std::vector<std::string> variables = {"APCP_surface", "T2D_2maboveground", "SPFH_2maboveground"};
    const unsigned n_threads = test_thread_count();
    const int n_vars = variables.size();

    std::vector<char> ok(n_rounds * n_vars * n_threads, false);
    std::vector<int> n_inserted(n_rounds * n_vars, 0);
    shared_cache cache;
    std::barrier round_end(n_threads);

    run_threads(n_threads, [&](unsigned t) {
        std::minstd_rand rng(t + 1);
        std::vector<int> order(n_vars);
        for (int v = 0; v < n_vars; ++v)
            order[v] = v;

        for (int r = 0; r < n_rounds; ++r) {
            std::shuffle(order.begin(), order.end(), rng);
            for (int v : order) {
                const int cell = r * n_vars + v;
                const double seed = cell * 10.0;
                key_type key{r * page_size, variables[v]};

                auto [slot, inserted] = cache.find_or_insert(key, true);
                buffer_type got;
                if (inserted) {
                    jitter(rng);
                    got = make_buffer(seed);
                    slot.fill(got, true);
                    // Exactly one thread writes each cell
                    ++n_inserted[cell];
                } else {
                    got = slot.get();
                }
                ok[cell * n_threads + t] = buffer_holds(got, seed);
            }
            round_end.arrive_and_wait();
        }
    });

    for (int cell = 0; cell < n_rounds * n_vars; ++cell) {
        EXPECT_EQ(n_inserted[cell], 1) << "round " << cell / n_vars << ", variable " << cell % n_vars;
        for (unsigned t = 0; t < n_threads; ++t)
            EXPECT_TRUE(ok[cell * n_threads + t])
                << "round " << cell / n_vars << ", variable " << cell % n_vars << ", thread " << t;
    }

    // Each round's inserts evicted every earlier round
    EXPECT_EQ(cache.cache.size(), variables.size());
    for (auto const& entry : cache.cache)
        EXPECT_EQ(entry.first.first, (n_rounds - 1) * page_size);
}

// Demonstrates a known hazard rather than a desired property, so it is
// disabled. To see the report, run
//   TSAN_OPTIONS=halt_on_error=1 test_netcdf_forcing_cache --gtest_also_run_disabled_tests --gtest_filter='*DISABLED_Eviction*'
// Without halt_on_error the test hangs after the report, as get() spins
// on whatever now occupies the freed slot's memory.
//
// find_or_insert() returns a reference into the map, but releases the
// lock before returning. An evicting insert of a later time index by
// another thread can then destroy the slot while the first thread is
// still about to get() (or fill()) it. In get_value() this is a thread
// preempted between find_or_insert() and get() while another thread
// moves on to the next page. The allocator commonly reuses the freed
// node for the newly inserted one, so the stale reference can alias the
// new key's slot and return that page's data instead of crashing.
TEST(SharedCacheTest, DISABLED_EvictionWhileAnotherThreadHoldsSlot)
{
    shared_cache cache;
    key_type old_key{0, "APCP_surface"};
    key_type new_key{24, "APCP_surface"};
    cache.find_or_insert(old_key, true).first.fill(make_buffer(0.0), true);

    std::atomic<bool> holding{false};
    std::atomic<bool> evicted{false};
    run_threads(2, [&](unsigned t) {
        if (t == 0) {
            cache_slot& slot = cache.find_or_insert(old_key, true).first;
            holding.store(true, std::memory_order_relaxed);
            while (!evicted.load(std::memory_order_relaxed))
                std::this_thread::yield();
            EXPECT_TRUE(buffer_holds(slot.get(), 0.0)); // use after free
        } else {
            while (!holding.load(std::memory_order_relaxed))
                std::this_thread::yield();
            cache.find_or_insert(new_key, true);
            evicted.store(true, std::memory_order_relaxed);
        }
    });
}

#endif // NGEN_WITH_NETCDF
