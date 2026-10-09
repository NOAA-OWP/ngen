#include "utilities/MemfdFileBuffer.hpp"

#include <cerrno>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <sys/mman.h>
#include <unistd.h>

namespace utils {

namespace {

[[noreturn]] void throw_errno(int err, std::string const& what) {
    throw std::system_error(err, std::generic_category(), what);
}

// memfd names are for debugging only (visible in /proc/self/fd links), and
// are limited to 249 bytes
std::string memfd_name_for(std::string const& source_path) {
    auto const slash = source_path.find_last_of('/');
    std::string name = "ngen:" + (slash == std::string::npos ? source_path : source_path.substr(slash + 1));
    if (name.size() > 249) {
        name.resize(249);
    }
    return name;
}

void write_all(int fd, char const* data, std::size_t size) {
    while (size > 0) {
        ssize_t const n = ::write(fd, data, size);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            throw_errno(errno, "write to memfd");
        }
        data += n;
        size -= static_cast<std::size_t>(n);
    }
}

} // namespace

// ---------------------------------------------------------------------------
// MemfdHandle

MemfdHandle::~MemfdHandle() {
    reset();
}

MemfdHandle::MemfdHandle(MemfdHandle&& other) noexcept
    : fd_(std::exchange(other.fd_, -1)) {}

MemfdHandle& MemfdHandle::operator=(MemfdHandle&& other) noexcept {
    if (this != &other) {
        reset();
        fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
}

std::string MemfdHandle::path() const {
    if (fd_ < 0) {
        return {};
    }
    return "/proc/self/fd/" + std::to_string(fd_);
}

void MemfdHandle::reset() noexcept {
    if (fd_ >= 0) {
        // Linux always releases the descriptor, even if close() reports an error
        ::close(fd_);
        fd_ = -1;
    }
}

// ---------------------------------------------------------------------------
// MemfdFileBuffer

MemfdFileBuffer::MemfdFileBuffer(std::string const& path)
    : source_path_(path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        throw std::runtime_error("MemfdFileBuffer: unable to open '" + path + "'");
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    if (in.bad() || ss.bad()) {
        throw std::runtime_error("MemfdFileBuffer: error reading '" + path + "'");
    }
    contents_ = std::move(ss).str();
}

MemfdHandle MemfdFileBuffer::make_fd() const {
    // Owning the descriptor from the start closes it if filling fails
    MemfdHandle handle(::memfd_create(memfd_name_for(source_path_).c_str(), 0));
    if (!handle) {
        throw_errno(errno, "memfd_create for '" + source_path_ + "'");
    }
    write_all(handle.fd(), contents_.data(), contents_.size());
    if (::lseek(handle.fd(), 0, SEEK_SET) < 0) {
        throw_errno(errno, "lseek on memfd");
    }
    return handle;
}

} // namespace utils
