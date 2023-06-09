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

    template<bool B>
    using bool_constant = std::integral_constant<bool, B>;

    // a C++17 conjunction impl
    template<bool... Bs>
    using conjunction = std::is_same<bool_pack<true, Bs...>, bool_pack<Bs..., true>>;

    // a C++17 disjunction impl
    template<bool... Bs>
    using disjunction = bool_constant<!conjunction<!Bs...>::value>;

    /**
     * Check that all types @c{Ts} are the same as @c{T}.
     * 
     * @tparam T Type to constrain to
     * @tparam Ts Types to check
     */
    template<typename T, typename... Ts>
    using all_is_same = conjunction<std::is_same<Ts, T>::value...>;

    /**
     * Checks that all types @c{From} are convertible to @c{T}
     * 
     * @tparam To Type to constrain to
     * @tparam From Types to check
     */
    template<typename To, typename... From>
    using all_is_convertible = conjunction<std::is_convertible<From, To>::value...>;

    /**
     * Checks that @c{From} is convertible to any types in @c{To}
     * 
     * @tparam From Type to constrain to
     * @tparam To Types to check
     */
    template<typename From, typename... To>
    using is_convertible_to_any = disjunction<std::is_convertible<From, To>::value...>;

    /**
     * Checks that @c{T} is the same as at least one of @c{Ts}
     * 
     * @tparam T Type to check
     * @tparam Ts Types to contrain to
     */
    template<typename T, typename... Ts>
    using is_same_to_any = disjunction<std::is_same<T, Ts>::value...>;

} // namespace traits

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

    /**
     * An axis view is akin to a std::span (or std::mdspan),
     * over either the vertical or horizontal axes.
     */
    struct axis_view;

    /**
     * Constructs a new mdvector object with
     * no dimensions defined.
     */
    mdvector() noexcept
        : m_data(std::vector<Tp>{})
        , m_stride(0) {};

    /**
     * Constructs a new mdvector object with
     * the given dimensions.
     * 
     * @param dimensions Size of dimensions (aka number of columns)
     */
    mdvector(size_type dimensions) noexcept
        : m_data(std::vector<Tp>{})
        , m_stride(dimensions) {};

    /**
     * Constructs a new mdvector object with the given dimensions,
     * allocates enough space to fit @c{n} elements, and default
     * constructs the values.
     *
     * @param dimensions Size of dimensions (aka number of columns)
     * @param n Initial size of container (aka number of rows)
     */
    mdvector(size_type dimensions, size_type n) noexcept
        : m_data(std::vector<Tp>{dimensions * n})
        , m_stride(dimensions) {};

    /**
     * Construct a new mdvector object using the values given.
     *
     * @note The nested initializer lists @b{must} be the same size,
     *       otherwise, they will be padded with default constructed
     *       values to ensure the dimensions of vector are invariant.
     * 
     * @param values An initializer list of initializer lists containing type @c{Tp}.
     */
    mdvector(std::initializer_list<std::initializer_list<Tp>> values) noexcept
    {
        // Get total size and figure out stride
        for (const std::initializer_list<Tp>& list : values) {
            const size_type lsize = list.size();
            this->m_data.reserve(lsize);

            if (this->m_stride < lsize) {
                this->m_stride = lsize;
            }
        }

        for (const auto& list : values) {
            const size_type lsize = list.size();
            size_type remainder = this->m_stride - lsize;

            // Push back values
            for (const auto& value : list) {
                this->m_data.push_back(value);
            }

            // Add any remainder values to ensure invariance
            while (remainder > 0) {
                this->m_data.emplace_back();
                remainder--;
            }
        }
    }

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
     * View a vertical axis as a sequential container (aka like a std::span).
     *
     * @note A vertical axis view can be thought of as columns
     *       of a multi-dimensional vector in matrix representation.
     * 
     * @param n vertical axis index
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

    /**
     * View a horizontal axis as a sequential container (aka like a std::span).
     *
     * @note A horizontal axis view can be thought of as rows
     *       of a multi-dimensional vector in matrix representation.
     * 
     * @param n horizontal axis index
     * @return axis_view 
     */
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
struct mdvector<Tp>::axis_view
{
    using reference = mdvector<Tp>::reference;
    using size_type = mdvector<Tp>::size_type;

    /**
     * An axis iterator. This is a generalization
     * of a typical std::vector<Tp> iterator,
     * but contains information about this view's
     * dimensions. This is so traversal can be
     * stride-based.
     */
    struct iterator;

    /**
     * Axis views are not default constructible.
     */
    axis_view() = delete;

    /**
     * Constructs a new axis_view object ranging from
     * the @c{start} to @c{end} pointers spanned with
     * rank @c{dimensions}.
     * 
     * @param start Starting pointer
     * @param end Ending pointer
     * @param dimensions rank/stride
     */
    axis_view(pointer start, pointer end, size_type dimensions)
        : m_start(start)
        , m_end(end)
        , m_stride(dimensions) {};

    /**
     * Returns a reference to a value within this axis.
     * @param n index of element
     * @return reference 
     * @throws std::out_of_range if @c{n} is greater than
     *                           the size of this axis.
     */
    reference at(size_type n)
    {
        pointer loc = m_start + (n * m_stride);
        if (loc >= m_end)
            throw std::out_of_range("axis_view index out of range");
    
        return *loc;
    }

    /**
     * Returns a reference to a value within this axis.
     * @note The bracket operator does not perform bounds checking.
     * @param n index of element
     * @return reference 
     */
    reference operator[](size_type n) noexcept
    {
        return *(m_start + (n * m_stride));
    }

    /**
     * Returns an iterator to the beginning of this axis.
     * @return iterator 
     */
    iterator begin() noexcept
    {
        return iterator(m_start, m_stride);
    }

    /**
     * Returns an iterator positioned at the end of this axis
     * (aka one past the last element).
     * @return iterator 
     */
    iterator end() noexcept
    {
        return iterator(m_end, m_stride);
    }

    /**
     * Returns the true size of this axis.
     * @return size_type 
     */
    size_type size() noexcept
    {
        return static_cast<size_type>(m_end - m_start);
    }

  private:
    pointer m_start;
    pointer m_end;
    size_type m_stride;
};

template<typename Tp>
struct mdvector<Tp>::axis_view::iterator
{
    using iterator_category = std::random_access_iterator_tag;
    using size_type         = typename mdvector<Tp>::size_type;
    using difference_type   = typename mdvector<Tp>::difference_type;
    using value_type        = typename mdvector<Tp>::value_type;
    using pointer           = typename mdvector<Tp>::pointer;
    using reference         = typename mdvector<Tp>::reference;

    iterator(pointer ptr, size_type dimensions)
        : m_ptr(ptr)
        , m_stride(dimensions) {};

    reference operator*() const
    {
        return *m_ptr;
    }

    pointer operator->()
    {
        return m_ptr;
    }

    iterator& operator++()
    {
        m_ptr += m_stride;
        return *this;
    }

    iterator& operator++(int)
    {
        m_ptr += m_stride;
        return *this;
    }

    friend bool operator==(const iterator& lhs, const iterator& rhs)
    {
        return lhs.m_ptr == rhs.m_ptr;
    }

    friend bool operator!=(const iterator& lhs, const iterator& rhs)
    {
        return lhs.m_ptr != rhs.m_ptr;
    }

  private:
    pointer m_ptr;
    size_type m_stride;
};

} // namespace io

#endif // NGEN_IO_MDVECTOR_HPP