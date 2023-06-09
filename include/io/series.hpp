#ifndef NGEN_IO_SERIES_HPP
#define NGEN_IO_SERIES_HPP

#include <stdexcept>
#include <vector>
#include <string>

namespace io {

/**
 * A container type for handling multi-dimensional arrays.
 *
 * A series is backed by a std::vector, with interleaved elements
 * defined by some @b{stride}. Elements are stored and retrieved
 * based on this stride, to represent an N-dimensional array.
 * The stride is defined at runtime.
 * 
 * @tparam Tp Element value type
 */
template<typename Tp>
class series
{
  public:
    using container       = std::vector<Tp>;
    using value_type      = Tp;
    using size_type       = typename container::size_type;
    using reference       = typename container::reference;
    using const_reference = typename container::const_reference;

    /**
     * Retrieve a reference to a value.
     * 
     * @param axis The axis (aka dimension) to access.
     * @param n The index of the value across the @c{axis}.
     * @return reference 
     */
    reference at(size_type n, size_type axis)
    {
        if (axis >= m_stride)
            throw std::out_of_range("series");
    
        return this->m_data.at(this->m_stride * n + axis);
    }

    /**
     * Retrieve a const reference to a value.
     * 
     * @param axis The axis (aka dimension) to access.
     * @param n The index of the value across the @c{axis}.
     * @return const_reference 
     */
    const_reference at(size_type n, size_type axis) const
    {
        if (axis >= m_stride)
            throw std::out_of_range("series axis out of range");
    
        return this->m_data.at(this->m_stride * n + axis);
    }

    void push_back(std::initializer_list<value_type> values)
    {
        if (values.size() > m_stride)
            throw std::runtime_error("too many values");

        for (const auto& value : values) {
            this->m_data.push_back(value);
        }
    }

  private:
    container m_data;
    size_type m_stride;
};

} // namespace io

#endif // NGEN_IO_SERIES_HPP