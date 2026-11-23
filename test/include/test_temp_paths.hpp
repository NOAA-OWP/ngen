#ifndef NGEN_TEST_TEMP_PATHS_HPP
#define NGEN_TEST_TEMP_PATHS_HPP

#include <cerrno>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>

#include <stdlib.h> // mkdtemp

#include <gtest/gtest.h>

/**
 * Locations for files that tests write.
 *
 * ctest runs each test both from its suite binary and from `test_all`, and with
 * `-j` those runs overlap, so tests must not write to fixed paths in shared
 * directories such as `::testing::TempDir()` or the working directory. Everything
 * here is therefore placed in a directory that mkdtemp() made for the current
 * process, which is removed with its contents when the process exits.
 *
 * Each run of a test (including repeats under `--gtest_repeat`) gets a directory
 * of its own within that, which also starts out empty, so tests need neither
 * clear nor clean up the files they write there.
 */
namespace ngen {
namespace test {

namespace detail {

/// Make a new directory, accessible only to this user, in @p parent, named @p prefix followed by random characters.
inline std::filesystem::path make_temp_dir(const std::filesystem::path& parent, const std::string& prefix)
{
    std::string name = (parent / (prefix + "XXXXXX")).string();
    if (::mkdtemp(name.data()) == nullptr) {
        throw std::system_error(errno, std::generic_category(), "mkdtemp(" + name + ")");
    }
    return name;
}

} // namespace detail

/**
 * This process's directory for test files, under `::testing::TempDir()`.
 *
 * Made on first use, and removed, with its contents, when the process exits.
 */
inline const std::filesystem::path& process_temp_dir()
{
    struct owned_dir {
        const std::filesystem::path path = detail::make_temp_dir(::testing::TempDir(), "ngen_test_");
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

/**
 * The running test's own directory, in this process's directory.
 *
 * Made on first use in each run of a test, and the same for the rest of that run.
 * Under MPI, other ranks can use rank 0's if rank 0 broadcasts its path.
 *
 * @throws std::logic_error if no test is running
 */
inline std::filesystem::path test_temp_dir()
{
    // Forgets the directory when the test that made it ends
    struct test_dir : ::testing::EmptyTestEventListener {
        std::filesystem::path path;
        void OnTestEnd(const ::testing::TestInfo&) override { path.clear(); }
    };
    static test_dir* const current = [] {
        auto* d = new test_dir;
        ::testing::UnitTest::GetInstance()->listeners().Append(d); // takes ownership
        return d;
    }();

    if (current->path.empty()) {
        if (::testing::UnitTest::GetInstance()->current_test_info() == nullptr) {
            throw std::logic_error("test_temp_dir() called outside a test; use temp_path()");
        }
        current->path = detail::make_temp_dir(process_temp_dir(), "test_");
    }
    return current->path;
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
