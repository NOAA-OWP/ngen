#ifndef NGEN_UTILITIES_MMAP_POINTER_HPP
#define NGEN_UTILITIES_MMAP_POINTER_HPP

#include <iterator>
#include <type_traits>

namespace ngen {

/**
 * Fancy pointer satisfying the following named requirements and their requirements:
 * - [NullablePointer](https://en.cppreference.com/w/cpp/named_req/NullablePointer)
 * - [LegacyRandomAccessIterator](https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator)
 * 
 * @tparam Tp type this pointer dereferences to
 */
template<typename Tp>
struct mmap_pointer
{
    // type aliases are required per LegacyIterator.

    using value_type        = Tp;
    using size_type         = std::size_t;
    using difference_type   = std::ptrdiff_t;
    using reference         = value_type&;
    using const_reference   = const value_type&;
    using pointer           = value_type*;
    using const_pointer     = const value_type*;
    using iterator_category = std::random_access_iterator_tag;

    explicit mmap_pointer(pointer ptr, std::string path)
      : path_(std::move(path))
      , ptr_(ptr){};

    // @requirement DefaultConstructible
    explicit mmap_pointer() noexcept
      : path_()
      , ptr_(nullptr){};

    // @requirement NullablePointer
    explicit mmap_pointer(std::nullptr_t) noexcept
      : mmap_pointer(){};

    // @requirement NullablePointer
    mmap_pointer& operator=(std::nullptr_t) noexcept
    {
        ptr_ = nullptr;
        return *this;
    }
    
    // @requirement CopyConstructible
    mmap_pointer(const mmap_pointer&) noexcept = default;

    // @requirement CopyAssignable
    mmap_pointer& operator=(const mmap_pointer&) noexcept = default;

    // @requirement MoveConstructible
    mmap_pointer(mmap_pointer&&) noexcept = default;

    // @requirement MoveConstructible
    mmap_pointer& operator=(mmap_pointer&&) noexcept = default;

    // @requirement Destructible
    ~mmap_pointer() noexcept = default;

    // @requirement Swappable
    void swap(mmap_pointer& other);

    operator bool() noexcept {
        return ptr_ == nullptr;
    }

    operator void*() noexcept {
        return static_cast<void*>(ptr_);
    }

    // @requirement LegacyRandomAccessIterator
    reference operator[](difference_type n) noexcept
    {
        return ptr_[n];
    }

    // @requirement LegacyRandomAccessIterator
    const_reference operator[](difference_type n) const noexcept
    {
        return ptr_[n];
    }

    // @requirement LegacyInputIterator
    reference operator*() noexcept
    {
        return *ptr_;
    }

    // @requirement LegacyInputIterator
    const_reference operator*() const noexcept
    {
        return *ptr_;
    }
    
    // @requirement LegacyInputIterator
    pointer operator->() noexcept
    {
        return ptr_;
    }

    // @requirement LegacyInputIterator
    const_pointer operator->() const noexcept
    {
        return ptr_;
    }

    // @requirement LegacyIterator
    mmap_pointer& operator++() noexcept
    {
        ++ptr_;
        return *this;
    }

    // @requirement LegacyInputIterator
    mmap_pointer& operator++(int) noexcept
    {
        mmap_pointer tmp = *this;
        ++(*this);
        return tmp;
    }

    // @requirmeent LegacyBidirectionalIterator
    mmap_pointer& operator--() noexcept
    {
        --ptr_;
        return *this;
    }

    // @requirmeent LegacyBidirectionalIterator
    mmap_pointer& operator--(int) noexcept
    {
        mmap_pointer tmp = *this;
        --(*this);
        return tmp;
    }

    // @requirement LegacyRandomAccessIterator
    mmap_pointer& operator+=(difference_type n) noexcept
    {
        ptr_ += n;
        return *this;
    }

    // @requirement LegacyRandomAccessIterator
    mmap_pointer operator+(difference_type n) const noexcept
    {
        mmap_pointer tmp = *this;
        tmp.ptr_ += n;
        return tmp;
    }

    // @requirement LegacyRandomAccessIterator
    mmap_pointer operator+(size_type n) const noexcept
    {
        mmap_pointer tmp = *this;
        tmp.ptr_ += n;
        return tmp;
    }

    // @requirement LegacyRandomAccessIterator
    friend mmap_pointer operator+(difference_type n, const mmap_pointer& a) noexcept
    {
        mmap_pointer tmp = a;
        tmp.ptr_ += n;
        return tmp;
    }

    // @requirement LegacyRandomAccessIterator
    mmap_pointer& operator-=(difference_type n) noexcept
    {
        ptr_ -= n;
        return *this;
    }

    // @requirement LegacyRandomAccessIterator
    mmap_pointer operator-(difference_type n) const noexcept
    {
        mmap_pointer tmp = *this;
        tmp.ptr_ -= n;
        return tmp;
    }

    // @requirement LegacyRandomAccessIterator
    friend mmap_pointer operator-(difference_type n, const mmap_pointer& a) noexcept
    {
        mmap_pointer tmp = a;
        tmp.ptr_ -= n;
        return tmp;
    }

    difference_type operator-(const mmap_pointer& rhs) const noexcept
    {
        return ptr_ - rhs.ptr_;
    }

    // @requirement NullablePointer
    bool operator==(std::nullptr_t)
    {
        return ptr_ == nullptr;
    }

    // @requirement NullablePointer
    friend bool operator==(std::nullptr_t, const mmap_pointer& rhs)
    {
        return rhs == nullptr;
    }

    // @requirement NullablePointer
    bool operator!=(std::nullptr_t)
    {
        return ptr_ != nullptr;
    }

    // @requirement NullablePointer
    friend bool operator!=(std::nullptr_t, const mmap_pointer& rhs)
    {
        return rhs != nullptr;
    }

    // @requirement EqualityComparable
    template<typename T, typename U>
    friend bool operator==(const mmap_pointer<T>& lhs, const mmap_pointer<U>& rhs) noexcept
    {
        if (std::is_same<T, U>::value) {
            return lhs.ptr_ == rhs.ptr_;
        }

        return false;
    }

    // @requirement LegacyInputIterator
    template<typename T, typename U>
    friend bool operator!=(const mmap_pointer<T>& lhs, const mmap_pointer<U>& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    // @requirement LegacyRandomAccessIterator
    friend bool operator<(const mmap_pointer& lhs, const mmap_pointer& rhs) noexcept
    {
        return lhs.ptr_ < rhs.ptr_;
    }

    // @requirement LegacyRandomAccessIterator
    friend bool operator>(const mmap_pointer& lhs, const mmap_pointer& rhs) noexcept
    {
        return lhs.ptr_ > rhs.ptr_;
    }

    // @requirement LegacyRandomAccessIterator
    friend bool operator<=(const mmap_pointer& lhs, const mmap_pointer& rhs) noexcept
    {
        return lhs.ptr_ <= rhs.ptr_;
    }
    
    // @requirement LegacyRandomAccessIterator
    friend bool operator>=(const mmap_pointer& lhs, const mmap_pointer& rhs) noexcept
    {
        return lhs.ptr_ >= rhs.ptr_;
    }

    // ------------------------------------------------------------------------

    const std::string& path() const noexcept
    {
        return path_;
    }

  private:
    std::string path_;
    pointer     ptr_;
};


} // namespace ngen

#endif // NGEN_UTILITIES_MMAP_POINTER_HPP
