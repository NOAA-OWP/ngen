#ifndef NGEN_TEST_TEMP_PATHS_HPP
#define NGEN_TEST_TEMP_PATHS_HPP

#include <filesystem>
#include <string>
#include <system_error>

#include <unistd.h>

#include <gtest/gtest.h>

/**
 * Locations for files that tests write.
 *
 * ctest runs each test both from its suite binary and from `test_all`, and with
 * `-j` those runs overlap. `::testing::TempDir()` is shared by every process (it
 * is `/tmp/` unless `TEST_TMPDIR` is set), so a name derived only from the test
 * would let one process overwrite or delete another's file mid-test. Everything
 * here is therefore placed in a directory belonging to the current process,
 * which is removed with its contents when the process exits.
 *
 * Within a process, each run of a test (including repeats under
 * `--gtest_repeat`) gets a directory of its own that starts out empty, so tests
 * need neither clear nor clean up the files they write there.
 */
namespace ngen {
namespace test {

/**
 * Directory for test files of the process with the given ID, under `::testing::TempDir()`.
 *
 * Not created here. Under MPI, every rank can name rank 0's directory by passing
 * rank 0's process ID (broadcast by the caller).
 */
inline std::filesystem::path process_temp_dir(long pid)
{
    return std::filesystem::path(::testing::TempDir()) / ("ngen_test_" + std::to_string(pid));
}

/**
 * This process's directory for test files.
 *
 * Created on first use and removed, with its contents, when the process exits.
 */
inline const std::filesystem::path& process_temp_dir()
{
    struct owned_dir {
        std::filesystem::path path;
        owned_dir() : path(process_temp_dir(static_cast<long>(::getpid())))
        {
            std::filesystem::create_directories(path);
        }
        ~owned_dir()
        {
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }
    };
    static const owned_dir dir;
    return dir.path;
}

/**
 * Path for a file of the given name in this process's directory, shared by every test in the process.
 *
 * For files made once per test suite, e.g. in SetUpTestSuite(). Tests should use test_temp_path().
 */
inline std::filesystem::path temp_path(const std::string& name)
{
    return process_temp_dir() / name;
}

namespace detail {

/// Counts test starts, so that each run of a test in a process can be told apart.
struct test_run_counter : ::testing::EmptyTestEventListener {
    long count = 0;
    void OnTestStart(const ::testing::TestInfo&) override { ++count; }
};

/// Number of the running test among those this process has started, counting from 1.
inline long current_test_run()
{
    // Registered on first use, during a test whose start it therefore did not see
    static test_run_counter* const counter = [] {
        auto* c = new test_run_counter;
        c->count = 1;
        ::testing::UnitTest::GetInstance()->listeners().Append(c); // takes ownership
        return c;
    }();
    return counter->count;
}

} // namespace detail

/**
 * Name, unique within the process, for the running test: `<run>.<suite>.<test>`.
 *
 * Every MPI rank runs the same sequence of tests, so all ranks get the same name.
 * Must be called while a test is running.
 */
inline std::string test_run_name()
{
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string name = std::to_string(detail::current_test_run()) + "." + info->test_suite_name()
                     + "." + info->name();
    for (auto& c : name) {
        if (c == '/') c = '_';   // parameterized test names contain '/'
    }
    return name;
}

/**
 * Directory of the running test within the directory of the process with the given ID.
 *
 * Not created here; for MPI ranks sharing rank 0's directory (see process_temp_dir(long)).
 */
inline std::filesystem::path test_temp_dir(long pid)
{
    return process_temp_dir(pid) / test_run_name();
}

/**
 * The running test's own directory, created empty on first use in each run of the test.
 */
inline std::filesystem::path test_temp_dir()
{
    auto dir = process_temp_dir() / test_run_name();
    std::filesystem::create_directories(dir);
    return dir;
}

/**
 * Path for a file of the given name in the running test's own directory.
 *
 * The same name gives the same path throughout a run of a test. Nothing exists
 * there until the test creates it.
 */
inline std::filesystem::path test_temp_path(const std::string& name)
{
    return test_temp_dir() / name;
}

} // namespace test
} // namespace ngen

#endif // NGEN_TEST_TEMP_PATHS_HPP
