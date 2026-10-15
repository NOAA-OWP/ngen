// Benchmarks for NetCDFPerFeatureDataProvider::cache_slot and
// NetCDFPerFeatureDataProvider::shared_cache.
//
// Run with --help for options. Each configuration is run for some
// discarded warm-up repetitions and then some measured repetitions; each
// repetition starts its threads outside the timed region and releases
// them together through a barrier. Per-repetition times are reported as
// median, min and max over the measured repetitions, and per-operation
// wait latencies as percentiles pooled over all measured repetitions.
//
// Benchmarks:
//   slot_handoff  One thread fills a fresh slot after a simulated read
//                 latency, while the others wait in get(). Reports time
//                 per round, and wake latency: from fill() until each
//                 waiter's get() returns.
//   slot_get      Every thread repeatedly get()s an already-filled slot,
//                 measuring the steady-state cost and its scaling.
//   cache_lookup  Every thread repeatedly finds existing keys in a
//                 populated shared_cache, measuring the shared-lock hit path.
//   cache_rounds  Threads advance through time pages in lockstep, as in
//                 get_value(): per page, each looks up every variable, each
//                 in its own shared_cache as in the provider, with evicting
//                 inserts; the elected thread allocates and fills a buffer
//                 after the simulated latency, and the others get().
//
// The cpu/wall column is the threads' total CPU time over wall time in
// the timed region; with threads waiting on a fill, it shows how many
// cores the waiting burns. It includes time spent in the harness's own
// barriers between rounds.

#include <NGenConfig.h>

#include "NetCDFPerFeatureDataProvider.hpp"

#include <algorithm>
#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

using Provider = data_access::NetCDFPerFeatureDataProvider;
using cache_slot = Provider::cache_slot;
using shared_cache = Provider::shared_cache;
using buffer_type = Provider::cache_buffer_type;
using key_type = Provider::cache_key_type;

using bench_clock = std::chrono::steady_clock;
static_assert(bench_clock::is_steady);
using nanoseconds = std::chrono::duration<double, std::nano>;

// Keep the compiler from discarding a computed value
template <typename T>
inline void do_not_optimize(T const& value)
{
#if defined(__GNUC__)
    asm volatile("" : : "g"(&value) : "memory");
#else
    static void const* volatile sink;
    sink = &value;
#endif
}

// Time steps per cache page, matching cache_slice_t_size
constexpr int page_size = 24;

enum class fill_mode { sleep, spin };

struct options {
    std::vector<std::string> benches = {"slot_handoff", "slot_get", "cache_lookup", "cache_rounds"};
    std::vector<unsigned> threads;
    std::vector<unsigned> fill_latency_us = {0, 10, 100, 1000};
    fill_mode mode = fill_mode::sleep;
    bool same_var_order = true;
    unsigned reps = 10;
    unsigned warmup = 2;
    unsigned rounds = 100;
    unsigned keys = 64;
    unsigned vars = 8;
    std::size_t ops = 100000;
    std::size_t buffer_doubles = page_size * 1000;
    bool csv = false;
};

// Simulate reading and decoding a page of forcing data; returns the
// latency actually incurred, which for sleep often exceeds the request
double simulate_fill_latency(unsigned us, fill_mode mode)
{
    auto start = bench_clock::now();
    if (us > 0) {
        auto deadline = start + std::chrono::microseconds(us);
        if (mode == fill_mode::sleep)
            std::this_thread::sleep_until(deadline);
        else
            while (bench_clock::now() < deadline) {}
    }
    return nanoseconds(bench_clock::now() - start).count();
}

buffer_type make_buffer(std::size_t n_doubles)
{
    return std::make_shared<std::vector<double>>(n_doubles, 1.0);
}

double thread_cpu_ns()
{
    timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

struct run_times {
    double wall_ns;
    double cpu_per_wall;
};

// Start n_threads threads running body(thread_index), release them
// together, and time from the first thread's release until the last
// one finishes. Thread start-up and teardown are outside the timing.
run_times run_threads(unsigned n_threads, std::function<void(unsigned)> const& body)
{
    std::barrier release(n_threads);
    std::vector<bench_clock::time_point> starts(n_threads), ends(n_threads);
    std::vector<double> cpu_ns(n_threads);

    std::vector<std::thread> threads;
    threads.reserve(n_threads);
    for (unsigned t = 0; t < n_threads; ++t)
        threads.emplace_back([&, t] {
            release.arrive_and_wait();
            starts[t] = bench_clock::now();
            double cpu_start = thread_cpu_ns();
            body(t);
            cpu_ns[t] = thread_cpu_ns() - cpu_start;
            ends[t] = bench_clock::now();
        });
    for (auto& thread : threads)
        thread.join();

    double wall_ns = nanoseconds(*std::max_element(ends.begin(), ends.end())
                                 - *std::min_element(starts.begin(), starts.end())).count();
    double total_cpu_ns = 0;
    for (double ns : cpu_ns)
        total_cpu_ns += ns;
    return {wall_ns, total_cpu_ns / wall_ns};
}

// Results of one repetition
struct rep_result {
    double time;                    // ns per operation, or ns per round
    double cpu_per_wall;
    double fill_ns = 0;             // mean actual simulated fill latency
    std::vector<double> wait_ns{};  // per-operation wait latencies, if any
};

rep_result bench_slot_handoff(options const& o, unsigned n_threads, unsigned latency_us)
{
    // Slots and buffers are set up outside the timed region, so a round
    // costs only the simulated latency, fill(), get() and the barrier
    auto slots = std::make_unique<cache_slot[]>(o.rounds);
    std::vector<buffer_type> buffers(o.rounds);
    for (auto& buffer : buffers)
        buffer = make_buffer(o.buffer_doubles);
    auto fill_times = std::make_unique<std::atomic<bench_clock::rep>[]>(o.rounds);
    std::vector<double> fill_ns(o.rounds);
    std::vector<std::vector<double>> wake_ns(n_threads);
    std::barrier round_start(n_threads);

    auto times = run_threads(n_threads, [&](unsigned t) {
        wake_ns[t].reserve(o.rounds);
        for (unsigned r = 0; r < o.rounds; ++r) {
            round_start.arrive_and_wait();
            if (r % n_threads == t) {
                fill_ns[r] = simulate_fill_latency(latency_us, o.mode);
                fill_times[r].store(bench_clock::now().time_since_epoch().count(), std::memory_order_relaxed);
                slots[r].fill(buffers[r], true);
            } else {
                auto called = bench_clock::now();
                auto got = slots[r].get();
                auto returned = bench_clock::now();
                do_not_optimize(got);
                // Visible here, since it was stored before fill()
                bench_clock::time_point filled{bench_clock::duration{fill_times[r].load(std::memory_order_relaxed)}};
                wake_ns[t].push_back(nanoseconds(returned - std::max(called, filled)).count());
            }
        }
    });

    rep_result result{times.wall_ns / o.rounds, times.cpu_per_wall};
    for (double ns : fill_ns)
        result.fill_ns += ns / o.rounds;
    for (auto const& samples : wake_ns)
        result.wait_ns.insert(result.wait_ns.end(), samples.begin(), samples.end());
    return result;
}

rep_result bench_slot_get(options const& o, unsigned n_threads, unsigned)
{
    cache_slot slot;
    slot.fill(make_buffer(o.buffer_doubles), true);

    auto times = run_threads(n_threads, [&](unsigned) {
        for (std::size_t i = 0; i < o.ops; ++i) {
            auto got = slot.get();
            do_not_optimize(got);
        }
    });
    return {times.wall_ns / o.ops, times.cpu_per_wall};
}

rep_result bench_cache_lookup(options const& o, unsigned n_threads, unsigned)
{
    shared_cache cache;
    std::vector<key_type> keys;
    for (unsigned k = 0; k < o.keys; ++k) {
        keys.push_back(int(k) * page_size);
        cache.find_or_insert(keys.back(), std::nullopt).first.fill(make_buffer(1), true);
    }

    // Precomputed per-thread key sequences keep the RNG out of the timed loop
    constexpr std::size_t sequence_length = 4096;
    std::vector<std::vector<unsigned>> sequences(n_threads, std::vector<unsigned>(sequence_length));
    for (unsigned t = 0; t < n_threads; ++t) {
        std::minstd_rand rng(t + 1);
        std::uniform_int_distribution<unsigned> pick(0, o.keys - 1);
        for (auto& k : sequences[t])
            k = pick(rng);
    }

    auto times = run_threads(n_threads, [&](unsigned t) {
        auto const& sequence = sequences[t];
        for (std::size_t i = 0; i < o.ops; ++i) {
            auto found = cache.find_or_insert(keys[sequence[i % sequence_length]], std::nullopt);
            do_not_optimize(found.first);
        }
    });
    return {times.wall_ns / o.ops, times.cpu_per_wall};
}

rep_result bench_cache_rounds(options const& o, unsigned n_threads, unsigned latency_us)
{
    auto caches = std::make_unique<shared_cache[]>(o.vars);
    std::vector<std::vector<double>> wait_ns(n_threads);
    std::vector<double> fill_ns_total(n_threads, 0.0);
    std::vector<unsigned> n_fills(n_threads, 0);
    // The end-of-round barrier keeps any thread from evicting a page
    // another thread still holds a slot reference into
    std::barrier round_end(n_threads);

    auto times = run_threads(n_threads, [&](unsigned t) {
        wait_ns[t].reserve(o.rounds * o.vars);
        for (unsigned r = 0; r < o.rounds; ++r) {
            for (unsigned j = 0; j < o.vars; ++j) {
                unsigned v = o.same_var_order ? j : (j + t) % o.vars;
                key_type key = int(r) * page_size;

                auto [slot, inserted] = caches[v].find_or_insert(key, key);
                if (inserted) {
                    fill_ns_total[t] += simulate_fill_latency(latency_us, o.mode);
                    slot.fill(make_buffer(o.buffer_doubles), true);
                    ++n_fills[t];
                } else {
                    auto called = bench_clock::now();
                    auto got = slot.get();
                    wait_ns[t].push_back(nanoseconds(bench_clock::now() - called).count());
                    do_not_optimize(got);
                }
            }
            round_end.arrive_and_wait();
        }
    });

    rep_result result{times.wall_ns / o.rounds, times.cpu_per_wall};
    double fill_total = 0;
    unsigned fills = 0;
    for (unsigned t = 0; t < n_threads; ++t) {
        fill_total += fill_ns_total[t];
        fills += n_fills[t];
        result.wait_ns.insert(result.wait_ns.end(), wait_ns[t].begin(), wait_ns[t].end());
    }
    result.fill_ns = fills ? fill_total / fills : 0;
    return result;
}

struct bench_spec {
    std::string name;
    std::string unit;           // of rep_result::time
    bool uses_fill_latency;
    std::function<rep_result(options const&, unsigned, unsigned)> run;
};

const std::vector<bench_spec> all_benches = {
    {"slot_handoff", "us/round", true,  bench_slot_handoff},
    {"slot_get",     "ns/op",    false, bench_slot_get},
    {"cache_lookup", "ns/op",    false, bench_cache_lookup},
    {"cache_rounds", "us/round", true,  bench_cache_rounds},
};

// Nearest-rank percentile of sorted samples
double percentile(std::vector<double> const& sorted, double p)
{
    if (sorted.empty())
        return 0;
    std::size_t rank = std::size_t(p / 100.0 * (sorted.size() - 1) + 0.5);
    return sorted[std::min(rank, sorted.size() - 1)];
}

void print_header(options const& o)
{
    if (o.csv) {
        std::cout << "bench,threads,fill_us,unit,time_p50,time_min,time_max,fill_actual_us,"
                     "wait_us_p50,wait_us_p90,wait_us_p99,wait_us_max,cpu_per_wall\n";
        return;
    }
    std::printf("%-13s %7s %7s %-8s %10s %10s %10s %9s | %8s %8s %8s %9s | %8s\n",
                "bench", "threads", "fill_us", "unit", "time_p50", "time_min", "time_max", "fill_act",
                "wait_p50", "wait_p90", "wait_p99", "wait_max", "cpu/wall");
}

void report(options const& o, bench_spec const& spec, unsigned n_threads, unsigned latency_us,
            std::vector<rep_result> const& reps)
{
    const double scale = spec.unit == "us/round" ? 1e-3 : 1.0;
    std::vector<double> times, cpu, fill, waits;
    for (auto const& rep : reps) {
        times.push_back(rep.time * scale);
        cpu.push_back(rep.cpu_per_wall);
        fill.push_back(rep.fill_ns * 1e-3);
        for (double ns : rep.wait_ns)
            waits.push_back(ns * 1e-3);
    }
    std::sort(times.begin(), times.end());
    std::sort(cpu.begin(), cpu.end());
    std::sort(fill.begin(), fill.end());
    std::sort(waits.begin(), waits.end());

    const double p50 = percentile(times, 50), min = times.front(), max = times.back();
    const double fill_actual = percentile(fill, 50);
    const double w50 = percentile(waits, 50), w90 = percentile(waits, 90), w99 = percentile(waits, 99);
    const double wmax = waits.empty() ? 0 : waits.back();
    const double cpu_per_wall = percentile(cpu, 50);

    if (o.csv) {
        std::cout << spec.name << ',' << n_threads << ',' << latency_us << ',' << spec.unit << ','
                  << p50 << ',' << min << ',' << max << ',' << fill_actual << ','
                  << w50 << ',' << w90 << ',' << w99 << ',' << wmax << ',' << cpu_per_wall << '\n';
        return;
    }
    std::printf("%-13s %7u %7s %-8s %10.3f %10.3f %10.3f %9s | ",
                spec.name.c_str(), n_threads, spec.uses_fill_latency ? std::to_string(latency_us).c_str() : "-",
                spec.unit.c_str(), p50, min, max,
                spec.uses_fill_latency ? std::to_string(int(fill_actual + 0.5)).c_str() : "-");
    if (waits.empty())
        std::printf("%8s %8s %8s %9s | ", "-", "-", "-", "-");
    else
        std::printf("%8.2f %8.2f %8.2f %9.2f | ", w50, w90, w99, wmax);
    std::printf("%8.2f\n", cpu_per_wall);
    std::fflush(stdout);
}

void print_environment(options const& o)
{
    const unsigned hw = std::thread::hardware_concurrency();
    std::ostringstream env;
    env << "hardware_concurrency " << hw
        << ", fill mode " << (o.mode == fill_mode::sleep ? "sleep" : "spin")
        << ", variable order " << (o.same_var_order ? "same" : "rotated")
        << ", reps " << o.reps << " (+" << o.warmup << " warm-up)"
        << ", rounds " << o.rounds << ", ops " << o.ops
        << ", buffer " << o.buffer_doubles << " doubles";
    std::cerr << "# " << env.str() << "\n";

#if defined(__SANITIZE_THREAD__) || defined(__SANITIZE_ADDRESS__)
    std::cerr << "# WARNING: built with a sanitizer; timings are not representative\n";
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer) || __has_feature(address_sanitizer)
    std::cerr << "# WARNING: built with a sanitizer; timings are not representative\n";
#endif
#endif
#if defined(__GNUC__) && !defined(__OPTIMIZE__)
    std::cerr << "# WARNING: built without optimization; timings are not representative\n";
#endif
    if (hw > 0 && std::any_of(o.threads.begin(), o.threads.end(), [hw](unsigned t) { return t > hw; }))
        std::cerr << "# WARNING: some thread counts exceed hardware_concurrency; get() busy-waits,"
                     " so oversubscribed waiters compete with the filling thread for cores\n";
}

template <typename T>
std::vector<T> parse_list(std::string const& text, std::function<T(std::string const&)> const& parse)
{
    std::vector<T> values;
    std::stringstream stream(text);
    std::string item;
    while (std::getline(stream, item, ','))
        if (!item.empty())
            values.push_back(parse(item));
    return values;
}

unsigned parse_unsigned(std::string const& text)
{
    return unsigned(std::stoul(text));
}

void usage(char const* argv0)
{
    std::cout <<
        "Usage: " << argv0 << " [options]\n"
        "  --bench LIST            benchmarks to run, from: slot_handoff,slot_get,cache_lookup,cache_rounds\n"
        "                          (default: all)\n"
        "  --threads LIST          thread counts (default: powers of 2 up to hardware_concurrency)\n"
        "  --fill-latency-us LIST  simulated latency of filling a slot, in microseconds (default: 0,10,100,1000)\n"
        "  --fill-mode sleep|spin  sleep models blocking I/O; spin models CPU-bound decoding and gives\n"
        "                          precise latencies (default: sleep)\n"
        "  --var-order same|rotated  cache_rounds: whether all threads look up variables in the same\n"
        "                          order, as get_value() callers do, or each starts at a different one\n"
        "                          (default: same)\n"
        "  --reps N                measured repetitions per configuration (default: 10)\n"
        "  --warmup N              discarded warm-up repetitions per configuration (default: 2)\n"
        "  --rounds N              rounds per repetition for slot_handoff and cache_rounds (default: 100)\n"
        "  --ops N                 operations per thread per repetition for slot_get and cache_lookup\n"
        "                          (default: 100000)\n"
        "  --keys N                cache_lookup: number of keys in the cache (default: 64)\n"
        "  --vars N                cache_rounds: variables looked up per round (default: 8)\n"
        "  --buffer-doubles N      size of each filled buffer (default: 24000, 24 steps x 1000 catchments)\n"
        "  --csv                   print CSV instead of a table\n";
}

options parse_options(int argc, char** argv)
{
    options o;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << "\n";
                std::exit(2);
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") { usage(argv[0]); std::exit(0); }
        else if (arg == "--bench")           o.benches = parse_list<std::string>(value(), [](std::string const& s) { return s; });
        else if (arg == "--threads")         o.threads = parse_list<unsigned>(value(), parse_unsigned);
        else if (arg == "--fill-latency-us") o.fill_latency_us = parse_list<unsigned>(value(), parse_unsigned);
        else if (arg == "--fill-mode")       o.mode = value() == "spin" ? fill_mode::spin : fill_mode::sleep;
        else if (arg == "--var-order")       o.same_var_order = value() != "rotated";
        else if (arg == "--reps")            o.reps = parse_unsigned(value());
        else if (arg == "--warmup")          o.warmup = parse_unsigned(value());
        else if (arg == "--rounds")          o.rounds = parse_unsigned(value());
        else if (arg == "--ops")             o.ops = std::stoull(value());
        else if (arg == "--keys")            o.keys = parse_unsigned(value());
        else if (arg == "--vars")            o.vars = parse_unsigned(value());
        else if (arg == "--buffer-doubles")  o.buffer_doubles = std::stoull(value());
        else if (arg == "--csv")             o.csv = true;
        else {
            std::cerr << "Unknown option " << arg << "\n";
            usage(argv[0]);
            std::exit(2);
        }
    }

    if (o.threads.empty()) {
        const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
        for (unsigned t = 1; t < hw; t *= 2)
            o.threads.push_back(t);
        o.threads.push_back(hw);
    }
    if (o.reps == 0 || o.rounds == 0 || o.ops == 0 || o.keys == 0 || o.vars == 0
        || std::find(o.threads.begin(), o.threads.end(), 0u) != o.threads.end()) {
        std::cerr << "Counts must be positive\n";
        std::exit(2);
    }
    return o;
}

} // namespace

int main(int argc, char** argv)
{
    const options o = parse_options(argc, argv);
    print_environment(o);
    print_header(o);

    for (auto const& name : o.benches) {
        auto spec = std::find_if(all_benches.begin(), all_benches.end(),
                                 [&](bench_spec const& b) { return b.name == name; });
        if (spec == all_benches.end()) {
            std::cerr << "Unknown benchmark " << name << "\n";
            return 2;
        }

        const std::vector<unsigned> no_latency = {0};
        for (unsigned latency_us : spec->uses_fill_latency ? o.fill_latency_us : no_latency) {
            for (unsigned n_threads : o.threads) {
                for (unsigned w = 0; w < o.warmup; ++w)
                    spec->run(o, n_threads, latency_us);

                std::vector<rep_result> reps;
                for (unsigned r = 0; r < o.reps; ++r)
                    reps.push_back(spec->run(o, n_threads, latency_us));
                report(o, *spec, n_threads, latency_us, reps);
            }
        }
    }
    return 0;
}
