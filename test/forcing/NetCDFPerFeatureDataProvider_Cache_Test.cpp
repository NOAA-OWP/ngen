#include <NGenConfig.h>

#if NGEN_WITH_NETCDF

#include "gtest/gtest.h"
#include "NetCDFPerFeatureDataProvider.hpp"
#include "StreamHandler.hpp"

#include <netcdf>

#include <algorithm>
#include <barrier>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <thread>
#include <vector>

// Concurrency tests for NetCDFPerFeatureDataProvider's page cache.
//
// When built with GCC or Clang, the test target compiles the provider
// itself with -fsanitize=thread, so these tests are meant to be judged
// by ThreadSanitizer's verdict as much as by their assertions.

namespace {

using Provider = data_access::NetCDFPerFeatureDataProvider;

unsigned test_thread_count()
{
    return std::clamp(std::thread::hardware_concurrency(), 4u, 16u);
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

} // namespace

// ------------------------------------------------------------- get_value()

TEST(ProviderConcurrencyTest, ConcurrentGetValueReadsCorrectValues)
{
    // Threads call get_value() on one provider in lockstep time steps, as
    // Layer::update_models() does, each through its own thread cache.
    // Every read spans two time steps, so reads at page boundaries look
    // up two pages. The barrier between steps stands in for the one at
    // the end of each parallel loop, which eviction relies on.
    const std::size_t n_cats = 8, n_times = 100, chunk = 24;
    const time_t start = 1500000000, stride = 3600;
    const std::string path = "./nc_concurrent_get_value.nc";
    auto value = [](std::size_t cat, std::size_t t) { return 100.0 * cat + t; };
    {
        netCDF::NcFile out(path, netCDF::NcFile::replace, netCDF::NcFile::nc4);
        auto cat_dim = out.addDim("catchment-id", n_cats);
        auto time_dim = out.addDim("time", n_times);
        std::vector<netCDF::NcDim> dims = {cat_dim, time_dim};

        std::vector<std::string> ids;
        std::vector<const char*> id_ptrs;
        for (std::size_t c = 0; c < n_cats; ++c)
            ids.push_back("cat-" + std::to_string(c));
        for (auto const& id : ids)
            id_ptrs.push_back(id.c_str());
        out.addVar("ids", netCDF::ncString, cat_dim).putVar(id_ptrs.data());

        std::vector<double> times(n_cats * n_times), vals(n_cats * n_times);
        for (std::size_t c = 0; c < n_cats; ++c) {
            for (std::size_t t = 0; t < n_times; ++t) {
                times[c * n_times + t] = start + t * stride;
                vals[c * n_times + t] = value(c, t);
            }
        }
        auto time_var = out.addVar("Time", netCDF::ncDouble, dims);
        time_var.putAtt("units", "seconds since 1970-01-01 00:00:00");
        time_var.putVar(times.data());
        auto temp_var = out.addVar("temp", netCDF::ncDouble, dims);
        temp_var.putAtt("units", "K");
        std::vector<std::size_t> chunk_sizes = {n_cats, chunk};
        temp_var.setChunking(netCDF::NcVar::nc_CHUNKED, chunk_sizes);
        temp_var.putVar(vals.data());
    }

    const unsigned n_threads = test_thread_count();
    std::vector<char> ok((n_times - 1) * n_threads, false);
    {
        Provider provider(path, start, start + n_times * stride, utils::getStdErr());
        std::barrier step_end(n_threads);

        run_threads(n_threads, [&](unsigned t) {
            for (std::size_t step = 0; step + 1 < n_times; ++step) {
                const std::size_t cat = (t + step) % n_cats;
                CatchmentAggrDataSelector selector("cat-" + std::to_string(cat), "temp",
                                                   start + step * stride, 2 * stride, "K");
                const double expected = (value(cat, step) + value(cat, step + 1)) / 2;
                ok[step * n_threads + t] = std::abs(provider.get_value(selector, data_access::MEAN) - expected) < 1e-9;
                step_end.arrive_and_wait();
            }
        });
    }
    std::remove(path.c_str());

    for (std::size_t step = 0; step + 1 < n_times; ++step)
        for (unsigned t = 0; t < n_threads; ++t)
            EXPECT_TRUE(ok[step * n_threads + t]) << "step " << step << ", thread " << t;
}

#endif // NGEN_WITH_NETCDF
