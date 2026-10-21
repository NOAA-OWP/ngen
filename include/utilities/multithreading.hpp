#ifndef NGEN_MULTITHREADING_HPP
#define NGEN_MULTITHREADING_HPP

#include <atomic>
#include <cassert>
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <utility>

namespace multithreading
{
    namespace details
    {
        // A string usable as a template argument, so that
        // library_mutex() can be keyed by a library's name
        template <std::size_t N>
        struct string_literal {
            constexpr string_literal(const char (&str)[N]) {
                for (std::size_t i = 0; i < N; ++i)
                    value[i] = str[i];
            }

            char value[N];
        };

        // The one mutex in the process for the library named @arg
        // name, created on first use. Exported, so that it stays one per
        // process even if ngen is built as shared libraries with hidden
        // visibility. Call library_mutex() rather than this.
        [[gnu::visibility("default")]] std::mutex& named_library_mutex(char const* name);
    }

    // The mutex serializing calls into the library named @arg Name,
    // which can't be called from more than one thread at a time. Every
    // call into that library, anywhere in the process, must hold it:
    //
    //     std::unique_lock netcdf_lock(multithreading::library_mutex<"netcdf">());
    //
    // "netcdf" is the NetCDF library, and the HDF5 library beneath it
    // unless that was built thread-safe, which can't be called from more
    // than one thread at a time even for different files.
    //
    // It's a plain std::mutex, so regions holding it must not nest.
    //
    // Each executable or shared library that calls this gets its own
    // copy of the reference below, but they all refer to the one mutex
    // that details::named_library_mutex() keeps for Name. A static
    // mutex here instead would be one per shared library unless
    // exported, which GCC won't do for a template specialized on a
    // string_literal.
    template <details::string_literal Name>
    std::mutex& library_mutex()
    {
        static std::mutex& mutex = details::named_library_mutex(Name.value);
        return mutex;
    }

    // Thread-safe holder for one page of cached data. Exactly one
    // thread fill()s it in; until then the slot is EMPTY, and
    // other threads calling get() wait for it
    //
    // The buffer is a plain shared_ptr published through state_,
    // rather than a std::atomic<std::shared_ptr>, so that readers
    // only load state_ and don't contend on the lock that common
    // implementations of the latter take on every load
    template <typename T>
    struct cache_slot {
        cache_slot() = default;

        cache_slot(cache_slot const&) = delete;
        cache_slot(cache_slot &&) = delete;

        // Get back a non-nullptr buffer pointer that can safely
        // be read from. If it's not already set, this may block
        // on another thread that will call fill() to set said
        // pointer
        std::shared_ptr<T> get();

        // Fill in this slot with the provided buffer pointer and
        // safely unblock any threads waiting for it. Must be
        // called by one thread with a non-null pointer, on a slot
        // that is EMPTY, or in ERROR to retry a failed fill
        void fill(std::shared_ptr<T> buffer_ptr, bool immediate_use);

    private:
        std::shared_ptr<T> ptr_ = nullptr;
        // ERROR marks a failed fill, which fill() may retry
        enum STATE : char { EMPTY=0, FILLED=1, HOT=2, ERROR=3 };
        std::atomic<int> state_ = STATE::EMPTY;
    };

    template <typename Key, typename T>
    struct shared_cache {
        // Get back a cache slot matching the @arg key. Exactly
        // one thread will return 'true' the first time `key` is
        // passed
        //
        // Keys are expected to increase monotonically as the
        // simulation advances, so a key is not looked up again
        // after it is evicted. If it is, it is inserted again,
        // and this returns 'true' again
        //
        // If this call inserts a slot and @arg eviction_floor is
        // set, first evict all slots whose key is below it.
        // Callers must not pass a floor above the key of any slot
        // another thread may still be using, which includes the
        // key being looked up. Eviction is handled here to take
        // advantage of the locking on the internal structure
        // necessary for insertion.
        std::pair<cache_slot<T>&, bool> find_or_insert(Key const& key, std::optional<Key> const& eviction_floor);

        // The keys of all slots currently in the cache
        std::set<Key> keys() const;

    private:
        std::map<Key, cache_slot<T>> cache;
        mutable std::shared_mutex mutex;
    };

    // Remove entries in the passed cache with keys less than
    // floor_index, and return them, so the caller
    // controls when they are destroyed. Nodes are spliced, so this
    // allocates and frees nothing. Returning the named local relies on
    // NRVO copy elision; where that isn't applied, the map is moved,
    // which is also constant time.
    template <typename MapType>
    MapType evict_stale_values(MapType& cache, typename MapType::key_type const& floor_index)
    {
        MapType evicted;
        auto eviction_iterator = cache.begin();
        auto end = cache.end();
        while (eviction_iterator != end && eviction_iterator->first < floor_index) {
            evicted.insert(evicted.end(), cache.extract(eviction_iterator++));
        }
        return evicted;
    }

    template <typename T>
    void cache_slot<T>::fill(std::shared_ptr<T> buffer_ptr, bool immediate_use)
    {
        assert(buffer_ptr != nullptr);
        assert(state_.load(std::memory_order_relaxed) == STATE::EMPTY ||
               state_.load(std::memory_order_relaxed) == STATE::ERROR);
        ptr_ = std::move(buffer_ptr);
        // Publish with a release store on state_ itself, rather than a
        // separate fence, since ThreadSanitizer doesn't model
        // atomic_thread_fence and would report readers of the buffer
        // as racing with this thread
        state_.store(immediate_use ? STATE::HOT : STATE::FILLED, std::memory_order_release);
    }

    template <typename T>
    std::shared_ptr<T> cache_slot<T>::get()
    {
        int state = STATE::EMPTY;
        while ((state = state_.load(std::memory_order_acquire)) == STATE::EMPTY) {
            // Just spin on whatever thread is doing the filling
            #pragma omp taskyield
        }

        // Release, so that readers who see HOT from this store rather
        // than from fill() synchronize with this thread, and so
        // transitively happen-after the filling thread's writes
        if (state == STATE::FILLED)
            state_.store(STATE::HOT, std::memory_order_release);

        return ptr_;
    }

    template <typename Key, typename T>
    auto shared_cache<Key, T>::find_or_insert(Key const& key, std::optional<Key> const& eviction_floor) -> std::pair<cache_slot<T>&, bool>
    {
        // Look up the cache slot for key - either cache_iter =
        // find() gets an extant slot, or this thread commits to
        // creating and filling a new slot
        std::shared_lock l(mutex);
        auto cache_iter = cache.find(key);

        if (cache_iter != cache.end())
            return {cache_iter->second, false};

        // Upgrade the lock and try to take responsibility for filling this entry in
        l.unlock();
        // Declared before the lock, so that evicted entries are destroyed
        // after it is released, keeping frees of their buffers outside it
        decltype(cache) evicted;
        std::unique_lock ul(mutex);

        // Re-check that some other thread didn't get here first
        cache_iter = cache.find(key);

        if (cache_iter != cache.end())
            return {cache_iter->second, false};

        if (eviction_floor) {
            assert(*eviction_floor <= key);
            // Evict while we hold the writer lock, and are
            // responsible for modifying the cache anyway. Do it
            // before the caller fills the new slot, to drop references
            // to old data before loading new data to limit memory
            // footprint.
            evicted = evict_stale_values(cache, *eviction_floor);
        }

        // This thread really is reponsible for creating it
        return {cache.try_emplace(key).first->second, true};
    }

    template <typename Key, typename T>
    auto shared_cache<Key, T>::keys() const -> std::set<Key>
    {
        std::shared_lock l(mutex);
        std::set<Key> result;
        for (auto const& entry : cache)
            result.insert(entry.first);
        return result;
    }
}

#endif // NGEN_MULTITHREADING_HPP
