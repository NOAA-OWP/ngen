#ifndef NGEN_IO_MDVECTOR_HPP
#define NGEN_IO_MDVECTOR_HPP

#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace io {

namespace traits
{
    template<bool...>
    struct bool_pack
    {};

    // a C++17 conjunction impl
    template<bool... Bs>
    using conjunction = std::is_same<bool_pack<true, Bs...>, bool_pack<Bs..., true>>;

    template<typename T, typename... Ts>
    using all_is_same = conjunction<std::is_same<Ts, T>::value...>;

    template<typename T, typename... Ts>
    using all_is_convertible = conjunction<std::is_convertible<Ts, T>::value...>;
}

/**
 * A container type for handling multi-dimensional data.
 *
 * An mdvector is backed by a std::vector, with interleaved elements
 * defined by some @b{stride} (aka its dimension).
 * 
 * Elements are stored and retrieved based on this stride,
 * to represent an N-dimensional array.
 * 
 * The stride is defined at runtime. Data is stored in row-major form.
 * 
 * @tparam Tp Element value type
 */
template<typename Tp>
class mdvector
{
  public:
    using container       = std::vector<Tp>;
    using value_type      = Tp;
    using size_type       = typename container::size_type;
    using difference_type = typename container::difference_type;
    using reference       = typename container::reference;
    using const_reference = typename container::const_reference;
    using pointer         = typename container::pointer;
    using const_pointer   = typename container::const_pointer;

    struct axis_view;
    struct axis_iterator;

    /**
     * Constructs a new mdvector object with
     * no dimensions defined.
     */
    mdvector() noexcept
        : m_data(std::vector<Tp>{})
        , m_stride(0) {};

    mdvector(size_type dimensions) noexcept
        : m_data(std::vector<Tp>{})
        , m_stride(dimensions) {};

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
            throw std::out_of_range("mdvector");
    
        return this->m_data.at(this->m_stride * n + axis);
    }

    /**
     * Retrieve a const reference to a value.
     * 
     * @param axis The axis (aka dimension) to access.
     * @param n The index of the value across the @c{axis}.
     * @return const_reference
     *
     * @throws std::out_of_range if @c{axis} is greater or equal to
     *                           the number of dimensions.
     *
     * @throws std::out_of_range if @c{n} is greater or equal to
     *                           the number of multi-dimensional
     *                           values.
     */
    const_reference at(size_type n, size_type axis) const
    {
        if (axis >= m_stride)
            throw std::out_of_range("mdvector axis out of range");
    
        return this->m_data.at(this->m_stride * n + axis);
    }

    /**
     * Add a multi-dimensional value to the back of this mdvector.
     * 
     * @tparam Args
     * @param args A list of values that are convertible to the value type
     *             of this mdvector.
     * 
     * @throws std::runtime_error if number of values is greater than
     *                            the number of dimensions.   
     */
    template<typename... Args,
        std::enable_if_t<traits::all_is_convertible<Tp, Args...>::value,
            bool> = true>
    void push_back(Args&&... args)
    {
        this->push_back({ static_cast<Tp>(std::forward<Args>(args))... });
    }

    /**
     * Add a multi-dimensional value to the back of this mdvector.
     * 
     * @param values A list of values. The size of this list must
     *               match the dimensions of the mdvector.
     *
     * @throws std::runtime_error if number of values is greater than
     *                            the number of dimensions.   
     */
    void push_back(std::initializer_list<value_type> values)
    {
        if (values.size() > m_stride)
            throw std::runtime_error("too many values");

        for (const auto& value : values) {
            this->m_data.push_back(value);
        }
    }

    /**
     * Returns the number of multi-dimensional values in this container.
     *
     * @note This is not the @b{true size} of the backing container.
     *       For the true size, see `mdvector<Tp>::true_size()`.
     * 
     * @return size_type 
     */
    size_type size() const noexcept
    {
        return this->true_size() / this->m_stride;
    }

    /**
     * Returns the number of elements within the backing container.
     *
     * @note This is the @b{true size} of the container. For the
     *       number of multi-dimensional values, see `mdvector<Tp>::size()`.
     * 
     * @return size_type 
     */
    size_type true_size() const noexcept
    {
        return this->m_data.size();
    }

    /**
     * Returns the number of dimensions this mdvector has.
     * 
     * @return size_type 
     */
    size_type rank() const noexcept
    {
        return this->m_stride;
    }

    /**
     * Sets the number of dimensions this mdvector has.
     *
     * @note If the new rank @c{r} causes the mdvector'
     *       true size to not be divisible by the new
     *       rank, then default constructed objects will
     *       be appended until the true size of the backing
     *       container is divisible by the new rank.
     * 
     * @param r Number of dimensions
     */
    void rank(size_type r) noexcept
    {
        this->m_stride = r;

        if (this->true_size() > 0) {
            // Handle case where only 1 element is in the mdvector
            const size_type remainder = this->true_size() > this->m_stride 
                ? this->true_size() % this->m_stride
                : this->m_stride - this->true_size();

            if (remainder != 0) {
                this->m_data.resize(this->true_size() + remainder);
            }
        }
    }

    /**
     * View an axis as a sequential container (aka like a std::span).
     *
     * @note An axis view can be thought of as a column
     *       of a multi-dimensional vector projected
     *       down to a 2D table.
     * 
     * @param n Axis index
     * @return axis_view 
     */
    axis_view vaxis(size_type n)
    {
        if (n >= this->m_stride)
            throw std::out_of_range("vertical axis view out of range");

        return axis_view(
            (&this->m_data.front()) + n,
             // n + 1 to get the address of the address
             // AFTER the last elements, see: std::end() for ref
            (&this->m_data.back()) + n + 1,
            this->m_stride
        );
    }

    axis_view haxis(size_type n)
    {
        if (n >= this->size())
            throw std::out_of_range("horizontal axis view out of range");

        return axis_view(
            (&this->m_data.front()) + (this->m_stride * n),
            (&this->m_data.front()) + (this->m_stride * n) + this->m_stride,
            1
        );
    }

  private:
    container m_data;
    size_type m_stride;
};

template<typename Tp>
struct mdvector<Tp>::axis_iterator
{
    using iterator_category = std::random_access_iterator_tag;
    using size_type         = typename mdvector<Tp>::size_type;
    using difference_type   = typename mdvector<Tp>::difference_type;
    using value_type        = typename mdvector<Tp>::value_type;
    using pointer           = typename mdvector<Tp>::pointer;
    using reference         = typename mdvector<Tp>::reference;

    axis_iterator(pointer ptr, size_type dimensions)
        : m_ptr(ptr)
        , m_stride(dimensions) {};

    reference operator*() const { return *m_ptr; }
    pointer operator->() { return m_ptr; }

    axis_iterator& operator++()
    {
        m_ptr += m_stride;
        return *this;
    }

    axis_iterator operator++(int)
    {
        axis_iterator tmp = *this;
        tmp.m_ptr += m_stride;
        return tmp;
    }

    friend bool operator==(const axis_iterator& lhs, const axis_iterator& rhs)
    {
        return lhs.m_ptr == rhs.m_ptr;
    }

    friend bool operator!=(const axis_iterator& lhs, const axis_iterator& rhs)
    {
        return lhs.m_ptr != rhs.m_ptr;
    }

  private:
    pointer m_ptr;
    size_type m_stride;
};

template<typename Tp>
struct mdvector<Tp>::axis_view
{
    using iterator  = mdvector<Tp>::axis_iterator;
    using reference = mdvector<Tp>::reference;
    using size_type = mdvector<Tp>::size_type;

    axis_view() = delete;

    axis_view(pointer start, pointer end, size_type dimensions)
        : m_start(start)
        , m_end(end)
        , m_stride(dimensions) {};

    reference at(size_type n)
    {
        pointer loc = m_start + (n * m_stride);
        if (loc >= m_end)
            throw std::out_of_range("axis_view index out of range");
    
        return *loc;
    }

    reference operator[](size_type n)
    {
        return *(m_start + (n * m_stride));
    }

    iterator begin() noexcept { return iterator(m_start, m_stride); }
    iterator end()   noexcept { return iterator(m_end, m_stride); }
    size_type size() noexcept { return static_cast<size_type>(m_end - m_start); }

  private:
    pointer m_start;
    pointer m_end;
    size_type m_stride;
};

} // namespace io

#endif // NGEN_IO_MDVECTOR_HPP