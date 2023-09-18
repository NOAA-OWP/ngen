#ifndef NGEN_UTILITIES_MMAP_ALLOCATOR_HPP
#define NGEN_UTILITIES_MMAP_ALLOCATOR_HPP

#include <string>
#include <iostream>

extern "C" {
    
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

}

#include "mmap_pointer.hpp"

namespace ngen {

template<typename Tp, typename PoolPolicy>
struct mmap_allocator
{
    using value_type      = Tp;
    using pointer         = mmap_pointer<Tp>;
    using const_pointer   = const mmap_pointer<Tp>;
    using size_type       = typename pointer::size_type;
    using difference_type = typename pointer::difference_type;

    explicit mmap_allocator(std::string directory) noexcept
      : dir_(std::move(directory)){};

    mmap_allocator() noexcept
        : mmap_allocator(PoolPolicy::default_directory()){};

    pointer allocate(size_type n)
    {
        const size_type mem_size = sizeof(value_type) * n;
        std::pair<int, std::string> finfo = PoolPolicy::open(dir_);
        const int fd = finfo.first;

        // Truncate mmap file to allocated size
        if (ftruncate(fd, mem_size) < 0) {
            throw std::bad_alloc();
        }

        // Map file into virtual memory
        void* ptr = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        // Close file descriptor
        PoolPolicy::close(fd);

        if (ptr == nullptr || ptr == MAP_FAILED) {
            throw std::bad_alloc();
        }

        return pointer{ static_cast<value_type*>(ptr), std::move(finfo.second) };
    }

    void deallocate(pointer p, size_type n) noexcept
    {
        munmap(static_cast<void*>(p), n);
    }

    template<typename T, typename U, typename TPool, typename UPool>
    friend bool operator==(const mmap_allocator<T, TPool>& lhs, const mmap_allocator<U, UPool> rhs)
    {
        if (std::is_same<TPool, UPool>::value) {
            return lhs.dir_ == rhs.dir_;
        }

        return false;
    }

    template<typename T, typename U, typename TPool, typename UPool>
    friend bool operator!=(const mmap_allocator<T, TPool>& lhs, const mmap_allocator<U, UPool> rhs)
    {
        if (std::is_same<TPool, UPool>::value) {
            return lhs.dir_ != rhs.dir_;
        }

        return true;
    }

  private:
    std::string dir_;
};

/**
 * A basic file pool that creates files from a directory.
 * Defaults to the system tempfs dir, which will be one of:
 * - Environment variables TMPDIR, TMP, TEMP, or TEMPDIR
 * - Or, `/tmp`.
 */
struct basic_pool {
    static std::pair<int, std::string> open(const std::string& directory)
    {
        std::string name = directory;
        name += "/ngen_mmap_XXXXXX";
        mkstemp(&name[0]);

        const int fd = ::open(name.c_str(), O_CREAT | O_RDWR | O_TRUNC);

        ::unlink(name.c_str());
        return { fd, name };
    }

    static void close(int fd)
    {
        ::close(fd);
    }

    static const char* default_directory() noexcept
    {
        // Per ISO/IEC 9945 (POSIX)
        for (auto& var : { "TMPDIR", "TMP", "TEMP", "TEMPDIR" }) {
            decltype(auto) env = std::getenv(var);
            if (env != nullptr) {
                return env;
            }
        }

        // Default
        return "/tmp";
    }
};

} // namespace ngen

#endif // NGEN_UTILITIES_MMAP_ALLOCATOR_HPP
