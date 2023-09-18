#ifndef NGEN_UTILITIES_MMAP_ALLOCATOR_HPP
#define NGEN_UTILITIES_MMAP_ALLOCATOR_HPP

#include <string>

extern "C" {
    
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

}

namespace ngen {

/**
 * @brief Memory mapped allocator.
 * 
 * @tparam Tp Type that is allocated
 * @tparam PoolPolicy Backing pool policy; see `ngen::basic_pool`
 */
template<typename Tp, typename PoolPolicy>
struct mmap_allocator
{
    using value_type      = Tp;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    /**
     * @brief Create a new mmap allocator with the given directory
     *        as the storage location.
     * 
     * @param directory Path to storage location
     */
    explicit mmap_allocator(std::string directory) noexcept
      : dir_(std::move(directory)){};

    /**
     * @brief Default construct a mmap allocator.
     *
     * @note Uses PoolPolicy::default_directory() as the storage location.
     * 
     */
    mmap_allocator() noexcept
        : mmap_allocator(PoolPolicy::default_directory()){};

    /**
     * @brief Allocate `n` objects of type `Tp`, aka `sizeof(Tp) * n`.
     * 
     * @param n Number of elements to allocate
     * @return pointer 
     */
    pointer allocate(size_type n)
    {
        const size_type mem_size = sizeof(value_type) * n;
        const int fd = PoolPolicy::open(dir_);

        // Truncate mmap file to allocated size
        if (ftruncate(fd, mem_size) < 0) {
            throw std::bad_alloc();
        }

        // Map file into virtual memory
        void* ptr = mmap(nullptr, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        // Close file descriptor
        PoolPolicy::close(fd);

        if (ptr == nullptr || ptr == MAP_FAILED) {
            throw std::bad_alloc();
        }

        return static_cast<pointer>(ptr);
    }

    /**
     * @brief Deallocate `n` elements starting at address `p`.
     * 
     * @param p Pointer to beginning address
     * @param n Number of elements to deallocate
     */
    void deallocate(pointer p, size_type n) noexcept
    {
        munmap(static_cast<void*>(p), sizeof(value_type) * n);
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
    /**
     * @brief Open a new pool file in `directory`
     * 
     * @param directory Storage location
     * @return int File descriptor
     */
    static int open(const std::string& directory)
    {
        std::string name = directory;
        name += "/ngen_mmap_XXXXXX";
        mkstemp(&name[0]);

        const int fd = ::open(name.c_str(), O_CREAT | O_RDWR | O_TRUNC);
    
        ::unlink(name.c_str());
        return fd;
    }

    /**
     * @brief Close the given file descriptor
     * 
     * @param fd File descriptor
     */
    static void close(int fd)
    {
        ::close(fd);
    }

    /**
     * @brief Get the default directory for this pool.
     *
     * @details
     * Defaults to "/tmp", or the first value set
     * in the following environment variables:
     *
     *     TMPDIR, TMP, TEMP, or TEMPDIR.
     * 
     * @return const char* Absolute file path
     */
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
