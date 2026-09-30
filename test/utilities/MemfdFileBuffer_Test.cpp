#include <gtest/gtest.h>

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <utilities/MemfdFileBuffer.hpp>

using utils::MemfdFileBuffer;
using utils::MemfdHandle;

namespace {

bool fd_is_open(int fd) {
    return ::fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

std::string read_path(std::string const& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    EXPECT_TRUE(in.good()) << "could not open " << path;
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string read_fd_from_start(int fd) {
    std::string out;
    char buf[4096];
    off_t off = 0;
    for (;;) {
        ssize_t const n = ::pread(fd, buf, sizeof buf, off);
        if (n <= 0) {
            EXPECT_EQ(n, 0) << "pread failed: errno " << errno;
            break;
        }
        out.append(buf, static_cast<std::size_t>(n));
        off += n;
    }
    return out;
}

class MemfdFileBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        static int counter = 0;
        dir_ = std::filesystem::temp_directory_path()
            / ("ngen_memfd_test_" + std::to_string(::getpid())) / std::to_string(counter++);
        std::filesystem::remove_all(dir_);
        std::filesystem::create_directories(dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(dir_);
    }

    std::string write_file(std::string const& name, std::string const& contents) {
        auto p = dir_ / name;
        std::ofstream out(p, std::ios::out | std::ios::binary | std::ios::trunc);
        out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        return p.string();
    }

    std::filesystem::path dir_;
};

} // namespace

TEST_F(MemfdFileBufferTest, LoadsFileContents) {
    std::string const text = "{\n  \"global\": { \"formulations\": [] }\n}\n";
    MemfdFileBuffer buffer(write_file("config.json", text));

    EXPECT_EQ(buffer.contents(), text);
}

TEST_F(MemfdFileBufferTest, BufferIsIndependentOfFileAfterLoad) {
    std::string const path = write_file("config.json", "original");
    MemfdFileBuffer buffer(path);
    write_file("config.json", "changed on disk");

    EXPECT_EQ(buffer.contents(), "original");
    auto h = buffer.make_fd();
    EXPECT_EQ(read_path(h.path()), "original");
}

TEST_F(MemfdFileBufferTest, MissingFileThrows) {
    EXPECT_THROW(MemfdFileBuffer((dir_ / "no_such_file").string()), std::runtime_error);
}

TEST_F(MemfdFileBufferTest, BinaryContentsRoundTrip) {
    std::string bytes;
    for (int i = 0; i < 3 * 256; ++i) {
        bytes.push_back(static_cast<char>(i % 256));
    }
    MemfdFileBuffer buffer(write_file("binary.dat", bytes));

    ASSERT_EQ(buffer.contents().size(), bytes.size());
    auto h = buffer.make_fd();
    EXPECT_EQ(read_path(h.path()), bytes);
}

TEST_F(MemfdFileBufferTest, EmptyFile) {
    MemfdFileBuffer buffer(write_file("empty", ""));

    EXPECT_TRUE(buffer.contents().empty());
    auto h = buffer.make_fd();
    ASSERT_TRUE(h);
    EXPECT_EQ(read_path(h.path()), "");
}

TEST_F(MemfdFileBufferTest, LargerThanOnePage) {
    std::string big(1 << 20, 'x');
    for (std::size_t i = 0; i < big.size(); i += 4093) {
        big[i] = static_cast<char>('a' + (i % 26));
    }
    MemfdFileBuffer buffer(write_file("big", big));

    auto h = buffer.make_fd();
    EXPECT_EQ(read_path(h.path()), big);
}

TEST_F(MemfdFileBufferTest, HandleExposesProcSelfPath) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    auto h = buffer.make_fd();

    ASSERT_TRUE(h);
    EXPECT_GE(h.fd(), 0);
    EXPECT_EQ(h.path(), "/proc/self/fd/" + std::to_string(h.fd()));
    EXPECT_TRUE(fd_is_open(h.fd()));
}

TEST_F(MemfdFileBufferTest, DescriptorIsPositionedAtStart) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    auto h = buffer.make_fd();

    char buf[8] = {};
    ASSERT_EQ(::read(h.fd(), buf, sizeof buf), 3);
    EXPECT_EQ(std::string(buf, 3), "abc");
}

TEST_F(MemfdFileBufferTest, PathCanBeOpenedRepeatedlyFromStart) {
    MemfdFileBuffer buffer(write_file("config.json", "hello world"));
    auto h = buffer.make_fd();

    EXPECT_EQ(read_path(h.path()), "hello world");
    EXPECT_EQ(read_path(h.path()), "hello world");
}

TEST_F(MemfdFileBufferTest, DistinctHandlesHaveDistinctDescriptors) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    auto h1 = buffer.make_fd();
    auto h2 = buffer.make_fd();
    auto h3 = buffer.make_fd();

    EXPECT_NE(h1.fd(), h2.fd());
    EXPECT_NE(h1.fd(), h3.fd());
    EXPECT_NE(h2.fd(), h3.fd());
    EXPECT_NE(h1.path(), h2.path());
    EXPECT_EQ(read_path(h1.path()), "abc");
    EXPECT_EQ(read_path(h2.path()), "abc");
    EXPECT_EQ(read_path(h3.path()), "abc");
}

TEST_F(MemfdFileBufferTest, DistinctHandlesHaveDistinctInodes) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    auto h1 = buffer.make_fd();
    auto h2 = buffer.make_fd();

    struct stat s1{}, s2{};
    ASSERT_EQ(::fstat(h1.fd(), &s1), 0);
    ASSERT_EQ(::fstat(h2.fd(), &s2), 0);
    EXPECT_NE(s1.st_ino, s2.st_ino);
}

TEST_F(MemfdFileBufferTest, OpenedPathRefersToHandlesInode) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    auto h = buffer.make_fd();

    int const opened = ::open(h.path().c_str(), O_RDONLY);
    ASSERT_GE(opened, 0);
    struct stat sh{}, so{};
    EXPECT_EQ(::fstat(h.fd(), &sh), 0);
    EXPECT_EQ(::fstat(opened, &so), 0);
    ::close(opened);
    EXPECT_EQ(sh.st_dev, so.st_dev);
    EXPECT_EQ(sh.st_ino, so.st_ino);
}

TEST_F(MemfdFileBufferTest, WritesThroughOneHandleAreNotSeenElsewhere) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    auto h1 = buffer.make_fd();
    auto h2 = buffer.make_fd();

    ASSERT_EQ(::pwrite(h1.fd(), "X", 1, 0), 1);

    EXPECT_EQ(read_path(h1.path()), "Xbc");
    EXPECT_EQ(read_path(h2.path()), "abc");
    EXPECT_EQ(buffer.contents(), "abc");
    EXPECT_EQ(read_path(buffer.make_fd().path()), "abc");
}

TEST_F(MemfdFileBufferTest, ReadingOneHandleDoesNotMoveAnother) {
    MemfdFileBuffer buffer(write_file("config.json", "abcdef"));
    auto h1 = buffer.make_fd();
    auto h2 = buffer.make_fd();

    char buf[8] = {};
    ASSERT_EQ(::read(h1.fd(), buf, sizeof buf), 6);
    ASSERT_EQ(::read(h2.fd(), buf, sizeof buf), 6);
    EXPECT_EQ(std::string(buf, 6), "abcdef");
}

TEST_F(MemfdFileBufferTest, DestructionClosesDescriptor) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    int fd;
    {
        auto h = buffer.make_fd();
        fd = h.fd();
        ASSERT_TRUE(fd_is_open(fd));
    }
    EXPECT_FALSE(fd_is_open(fd));
}

TEST_F(MemfdFileBufferTest, ResetClosesDescriptorAndEmptiesHandle) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    auto h = buffer.make_fd();
    int const fd = h.fd();

    h.reset();
    EXPECT_FALSE(h);
    EXPECT_EQ(h.fd(), -1);
    EXPECT_EQ(h.path(), "");
    EXPECT_FALSE(fd_is_open(fd));

    // Resetting an empty handle is harmless
    h.reset();
    EXPECT_FALSE(h);
}

TEST_F(MemfdFileBufferTest, DefaultHandleIsEmpty) {
    MemfdHandle h;
    EXPECT_FALSE(h);
    EXPECT_EQ(h.fd(), -1);
    EXPECT_EQ(h.path(), "");
}

TEST_F(MemfdFileBufferTest, MoveConstructionTransfersOwnership) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    auto h1 = buffer.make_fd();
    int const fd = h1.fd();

    MemfdHandle h2(std::move(h1));
    EXPECT_FALSE(h1);
    EXPECT_EQ(h2.fd(), fd);
    EXPECT_TRUE(fd_is_open(fd));

    h1.reset();
    EXPECT_TRUE(fd_is_open(fd));
}

TEST_F(MemfdFileBufferTest, MoveAssignmentReleasesPreviousDescriptor) {
    MemfdFileBuffer buffer(write_file("config.json", "abc"));
    auto h1 = buffer.make_fd();
    auto h2 = buffer.make_fd();
    int const fd1 = h1.fd();
    int const fd2 = h2.fd();

    h1 = std::move(h2);
    EXPECT_FALSE(fd_is_open(fd1));
    EXPECT_EQ(h1.fd(), fd2);
    EXPECT_FALSE(h2);
}

TEST_F(MemfdFileBufferTest, HandleMayOutliveBuffer) {
    MemfdHandle h;
    {
        MemfdFileBuffer buffer(write_file("config.json", "survives"));
        h = buffer.make_fd();
    }
    ASSERT_TRUE(h);
    EXPECT_EQ(read_path(h.path()), "survives");
    int const fd = h.fd();
    h.reset();
    EXPECT_FALSE(fd_is_open(fd));
}

TEST_F(MemfdFileBufferTest, ConcurrentCreateAndRelease) {
    std::string const text = "concurrent contents";
    MemfdFileBuffer buffer(write_file("config.json", text));
    constexpr std::size_t threads = 8;
    constexpr std::size_t per_thread = 50;

    std::vector<std::vector<MemfdHandle>> held(threads);
    std::vector<int> mismatches(threads, 0);
    {
        std::vector<std::thread> workers;
        for (std::size_t t = 0; t < threads; ++t) {
            workers.emplace_back([&, t] {
                for (std::size_t i = 0; i < per_thread; ++i) {
                    // One handle released immediately, one kept
                    auto transient = buffer.make_fd();
                    if (read_fd_from_start(transient.fd()) != text) {
                        ++mismatches[t];
                    }
                    held[t].push_back(buffer.make_fd());
                }
            });
        }
        for (auto& w : workers) {
            w.join();
        }
    }
    for (int m : mismatches) {
        EXPECT_EQ(m, 0);
    }
    for (auto const& per : held) {
        ASSERT_EQ(per.size(), per_thread);
        for (auto const& h : per) {
            EXPECT_EQ(read_fd_from_start(h.fd()), text);
        }
    }

    {
        std::vector<std::thread> workers;
        for (std::size_t t = 0; t < threads; ++t) {
            workers.emplace_back([&, t] { held[t].clear(); });
        }
        for (auto& w : workers) {
            w.join();
        }
    }
}
