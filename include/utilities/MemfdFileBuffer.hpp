#pragma once

#include <string>
#include <string_view>

namespace utils {

class MemfdFileBuffer;

/**
 * Owning RAII handle for one memfd created by MemfdFileBuffer::make_fd().
 *
 * Each handle refers to its own memfd, a distinct inode filled with the
 * contents of the creating buffer. Other code, including code that only
 * accepts a file path, can open it via path(). Destroying or resetting the
 * handle closes the descriptor; the kernel frees the memfd once every
 * descriptor referring to it is closed.
 *
 * Handles are move-only, and are independent of the MemfdFileBuffer that
 * created them.
 */
class MemfdHandle {
public:
    MemfdHandle() noexcept = default;
    ~MemfdHandle();

    MemfdHandle(MemfdHandle&& other) noexcept;
    MemfdHandle& operator=(MemfdHandle&& other) noexcept;

    MemfdHandle(MemfdHandle const&) = delete;
    MemfdHandle& operator=(MemfdHandle const&) = delete;

    /// The raw descriptor, or -1 for an empty handle
    int fd() const noexcept { return fd_; }

    /// "/proc/self/fd/NNN" for this descriptor; empty string for an empty handle
    std::string path() const;

    explicit operator bool() const noexcept { return fd_ >= 0; }

    /// Close the descriptor now and leave this handle empty
    void reset() noexcept;

private:
    friend class MemfdFileBuffer;

    explicit MemfdHandle(int fd) noexcept : fd_(fd) {}

    int fd_ = -1;
};

/**
 * Holds the complete contents of a file in memory, and hands out distinct
 * memfds filled from those contents. make_fd() may be called concurrently
 * from multiple threads.
 */
class MemfdFileBuffer {
public:
    /// Read the whole file at `path`. Throws std::runtime_error if it cannot be read.
    explicit MemfdFileBuffer(std::string const& path);

    std::string const& source_path() const noexcept { return source_path_; }
    std::string_view contents() const noexcept { return contents_; }

    /**
     * Create a new memfd, a distinct inode holding the buffer contents,
     * positioned at offset 0. Throws std::system_error on failure.
     */
    MemfdHandle make_fd() const;

private:
    std::string source_path_;
    std::string contents_;
};

} // namespace utils
