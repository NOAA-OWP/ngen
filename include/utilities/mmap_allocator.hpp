#ifndef NGEN_UTILITIES_MMAP_ALLOCATOR_HPP
#define NGEN_UTILITIES_MMAP_ALLOCATOR_HPP

#include <cstring>
#include <memory>
#include <system_error>
#include <sys/mman.h>

namespace ngen {

/**
 * Reference-counted mmap-backed pointer.
 * 
 * @tparam Tp element type
 */
template<typename Tp>
struct mmap_pointer
{
    using element_type    = Tp;
    using pointer         = Tp*;
    using const_pointer   = const Tp*;
    using reference       = Tp&;
    using const_reference = const Tp&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    constexpr mmap_pointer(pointer ptr, size_type len) noexcept
      : ptr_(make_mmap_pointer(ptr, len)){};

    reference       operator*()        noexcept {return *ptr_;}
    const_reference operator*()  const noexcept {return *ptr_;}
    pointer         operator->()       noexcept {return ptr_.get();}
    const_pointer   operator->() const noexcept {return ptr_.get();}

  private:
    /**
     * Get a lambda-defined `munmap` deleter that accepts a `pointer`.
     * 
     * @param len Size of the pointer that'll be passed to `munmap`
     * @return lambda function taking 1 `pointer` argument.
     */
    static constexpr auto munmap_(size_type len)
    {
        return [=](pointer ptr){munmap(ptr, len);};
    }

    /**
     * Create a mmap shared_ptr.
     * 
     * @param ptr Pointer to mmap'd region
     * @param len Size of mmap'd region
     * @return std::shared_ptr<element_type>
     */
    static constexpr auto make_mmap_pointer(pointer ptr, size_type len)
    {
        return std::shared_ptr<element_type>{ptr, munmap_(len)};
    }

    std::shared_ptr<element_type> ptr_;
};

template<typename Tp, typename BackendPolicy>
struct mmap_allocator
{
    using value_type      = Tp;
    using pointer         = mmap_pointer<value_type>;
    using const_pointer   = const mmap_pointer<value_type>;
    using size_type       = typename pointer::size_type;
    using difference_type = typename pointer::difference_type;

    using object_size = std::integral_constant<size_type, sizeof(value_type)>;

    explicit mmap_allocator(const std::string& directory);

    mmap_allocator() noexcept
        : mmap_allocator(BackendPolicy::default_directory){};
    
    ~mmap_allocator() noexcept;

    pointer allocator(size_type n);
    void    deallocate(pointer p, size_type n);
};

struct tmpfs_backend {
    static constexpr const char* default_directory = "/tmp/";
};

} // namespace ngen

#endif // NGEN_UTILITIES_MMAP_ALLOCATOR_HPP
