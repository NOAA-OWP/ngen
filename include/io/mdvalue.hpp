#ifndef NGEN_IO_MDVALUE_HPP
#define NGEN_IO_MDVALUE_HPP

#include <initializer_list>
#include <vector>
#include <climits>
#include <cstdint>

namespace io {

#warning NO DOCUMENTATION
template<typename T>
class mdvalue
{
  public:
    using key_type   = uint64_t;
    using value_type = T;
    using size_type  = std::size_t;
    using ilist_type = std::initializer_list<size_type>;

    #warning NO DOCUMENTATION
    mdvalue(ilist_type index) noexcept
        : m_index(mdvalue::encode(index))
        , m_value() {};

    #warning NO DOCUMENTATION
    mdvalue(ilist_type index, value_type value) noexcept
        : m_index(mdvalue::encode(index))
        , m_value(value) {};

    mdvalue()                          = default;
    mdvalue(const mdvalue&)            = default;
    mdvalue& operator=(const mdvalue&) = default;
    mdvalue(mdvalue&&)                 = default;
    mdvalue& operator=(mdvalue&&)      = default;

    #warning NO DOCUMENTATION
    const mdvalue& operator=(value_type v) const noexcept
    {
        this->m_value = v;
        return *this;
    }

    #warning NO DOCUMENTATION
    operator T() const
    {
        return this->m_value;
    }

    #warning NO DOCUMENTATION
    bool operator<(const mdvalue& rhs)  const noexcept
    {
        return this->m_value < rhs.m_value;
    }

    #warning NO DOCUMENTATION
    bool operator<=(const mdvalue& rhs) const noexcept
    {
        return this->m_value <= rhs.m_value;
    }

    #warning NO DOCUMENTATION
    bool operator>(const mdvalue& rhs)  const noexcept
    {
        return this->m_value > rhs.m_value;
    }

    #warning NO DOCUMENTATION
    bool operator>=(const mdvalue& rhs) const noexcept
    {
        return this->m_value >= rhs.m_value;
    }

    #warning NO DOCUMENTATION
    bool operator==(const mdvalue& rhs) const noexcept
    {
        return this->m_value == rhs.m_value;
    }

    #warning NO DOCUMENTATION
    bool operator!=(const mdvalue& rhs) const noexcept
    {
        return this->m_value != rhs.m_value;
    }

  private:
    friend std::equal_to<mdvalue>;
    friend std::not_equal_to<mdvalue>;
    friend std::greater<mdvalue>;
    friend std::less<mdvalue>;
    friend std::greater_equal<mdvalue>;
    friend std::less_equal<mdvalue>;

    #warning NO DOCUMENTATION
    static uint64_t encode(const std::vector<size_type>& v)
    {
        uint64_t index = 0;
        const size_type dsize = v.size();
        const size_type checkbits = (sizeof(uint64_t) * CHAR_BIT) / 3;
        for (uint64_t i = 0; i < checkbits; i++) {
            for (size_type vi = 0; vi < dsize; vi++) {
                index |= (v.at(vi) & static_cast<uint64_t>(1) << i) << ((dsize - 1) * i + vi);
            }
        }

        return index;
    }

    #warning NO DOCUMENTATION
    static std::vector<size_type> decode(uint64_t key, size_type rank)
    {
        std::vector<size_type> indices(rank);
    
        const size_type checkbits = (sizeof(uint64_t) * CHAR_BIT) / 3;
        for (size_type i = 0; i < checkbits; i++) {
            const size_type shift_fwd = rank * i;
            const size_type shift_bck = (rank - 1) * i;
            for (size_type j = 0; j < rank; j++) {
                indices[j] |= (key & (static_cast<uint64_t>(1) << (shift_fwd + j))) >> (shift_bck + j);
            }
        }

        return indices;
    }

    #warning NO DOCUMENTATION
    key_type   m_index;

    #warning NO DOCUMENTATION
    size_type  m_rank;

    #warning NO DOCUMENTATION
    mutable value_type m_value;
};

} // namespace io

#warning NO DOCUMENTATION
namespace std {

template<typename T>
struct equal_to<io::mdvalue<T>>
{
    constexpr bool operator()(const io::mdvalue<T>& lhs, const io::mdvalue<T>& rhs) const noexcept
    { return lhs.m_index == rhs.m_index; }

    constexpr bool operator()(const io::mdvalue<T>& lhs, const std::vector<typename io::mdvalue<T>::size_type>& rhs) const noexcept
    { return lhs.m_index == io::mdvalue<T>::encode(rhs); }
};

template<typename T>
struct not_equal_to<io::mdvalue<T>>
{
    constexpr bool operator()(const io::mdvalue<T>& lhs, const io::mdvalue<T>& rhs) const noexcept
    { return lhs.m_index != rhs.m_index; }

    constexpr bool operator()(const io::mdvalue<T>& lhs, const std::vector<typename io::mdvalue<T>::size_type>& rhs) const noexcept
    { return lhs.m_index != io::mdvalue<T>::encode(rhs); }
};

template<typename T>
struct greater<io::mdvalue<T>>
{
    constexpr bool operator()(const io::mdvalue<T>& lhs, const io::mdvalue<T>& rhs) const noexcept
    { return lhs.m_index > rhs.m_index;  }

    constexpr bool operator()(const io::mdvalue<T>& lhs, const std::vector<typename io::mdvalue<T>::size_type>& rhs) const noexcept
    { return lhs.m_index > io::mdvalue<T>::encode(rhs);  }
};

template<typename T>
struct less<io::mdvalue<T>>
{
    constexpr bool operator()(const io::mdvalue<T>& lhs, const io::mdvalue<T>& rhs) const noexcept
    { return lhs.m_index < rhs.m_index;  }

    constexpr bool operator()(const io::mdvalue<T>& lhs, const std::vector<typename io::mdvalue<T>::size_type>& rhs) const noexcept
    { return lhs.m_index < io::mdvalue<T>::encode(rhs);  }
};

template<typename T>
struct greater_equal<io::mdvalue<T>>
{
    constexpr bool operator()(const io::mdvalue<T>& lhs, const io::mdvalue<T>& rhs) const noexcept
    { return lhs.m_index >= rhs.m_index; }

    constexpr bool operator()(const io::mdvalue<T>& lhs, const std::vector<typename io::mdvalue<T>::size_type>& rhs) const noexcept
    { return lhs.m_index >= io::mdvalue<T>::encode(rhs); }
};

template<typename T>
struct less_equal<io::mdvalue<T>>
{
    constexpr bool operator()(const io::mdvalue<T>& lhs, const io::mdvalue<T>& rhs) const noexcept
    { return lhs.m_index <= rhs.m_index; }

    constexpr bool operator()(const io::mdvalue<T>& lhs, const std::vector<typename io::mdvalue<T>::size_type>& rhs) const noexcept
    { return lhs.m_index <= io::mdvalue<T>::encode(rhs); }
};

} // namespace std

#endif // NGEN_IO_MDVALUE_HPP