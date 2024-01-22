#ifndef NGEN_MDARRAY_DEFINITION_HPP
#define NGEN_MDARRAY_DEFINITION_HPP

#include <type_traits>
#include <vector>

#include <boost/core/span.hpp>

namespace ngen {

template<typename T>
class mdarray
{
  public:
    using value_type      = T;
    using container_type  = std::vector<value_type>;
    using size_type       = typename container_type::size_type;
    using difference_type = typename container_type::difference_type;
    using reference       = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using pointer         = typename container_type::pointer;
    using const_pointer   = typename container_type::const_pointer;

    struct iterator;
    friend iterator;

  private:
    template<typename Tp>
    inline size_type max_size(Tp shape)
    {
        size_type max = 1;
        for (size_type dimension : shape) {
            max *= dimension;
        }
        return max;
    }

  public:

    mdarray() = default;

    mdarray(const boost::span<const size_type> dsizes)
        : m_shape(dsizes.begin(), dsizes.end())
        , m_data(this->max_size(dsizes)) {};

    constexpr mdarray(std::initializer_list<size_type> dsizes)
        : m_shape(dsizes.begin(), dsizes.end())
        , m_data(this->max_size(dsizes)) {};

    /**
     * Retrieve a reference to the value at the given index.
     *
     * @example
     * mdarray<int> x({2, 2}); // rank 2
     * x.at({0, 0});           // get the value at (0, 0) in a 2D array.
     * x.at({1, 1}) = 3;       // set the value at (1, 1) in a 2D array.
     *
     * @param n Index list
     * @return reference 
     */
    reference at(const boost::span<const size_type> n)
    {
        this->bounds_check(n);

        return this->m_data.at(this->index(n));
    }

    /**
     * Retrieve a reference to the value at the given index.
     *
     * @example
     * mdarray<int> x({2, 2}); // rank 2
     * x.at({0, 0});           // get the value at (0, 0) in a 2D array.
     * x.at({1, 1}) = 3;       // set the value at (1, 1) in a 2D array.
     *
     * @param n Index list
     * @return reference 
     */
    reference operator[](const boost::span<const size_type> n) noexcept
    {
        return this->m_data[this->index(n)];
    }

    /**
     * Retrieve const reference to the value at the given index.
     *
     * @example
     * mdarray<int> x({1, 1}); // rank 2
     * x.at({0, 0});           // get the value at (0, 0) in a 2D array.
     *
     * @param n Index list
     * @return reference 
     */
    const_reference at(const boost::span<const size_type> n) const
    {
        this->bounds_check(n);

        return this->m_data.at(this->index(n));
    }

    /**
     * Retrieve const reference to the value at the given index.
     *
     * @example
     * mdarray<int> x({1, 1}); // rank 2
     * x.at({0, 0});           // get the value at (0, 0) in a 2D array.
     *
     * @param n Index list
     * @return reference 
     */
    const_reference operator[](const boost::span<const size_type> n) const noexcept
    {
        return this->m_data[this->index(n)];
    }

    /**
     * Insert a multi-dimensonal value at the given index.
     * 
     * @param n A multi-dimensional index with the same size as `m_shape`.
     * @param value The value to set at the given index.
     */
    void insert(const boost::span<const size_type> n, value_type value)
    {
        this->bounds_check(n);

        this->m_data[this->index(n)] = value;
    }

    void insert(std::initializer_list<std::pair<const boost::span<const size_type>, value_type>> args)
    {
        for (const auto& arg : args) {
            this->insert(arg.first, arg.second);
        }
    }

    /**
     * Get the size of allocated values in this mdarray
     * (aka the allocated elements in the backing vector)
     * @return size_type
     */
    size_type size() const noexcept
    {
        return this->m_data.size();
    }

    iterator begin() const noexcept {
        return iterator(*this, 0);
    }

    iterator end() const noexcept {
        return iterator(*this, this->m_data.size());
    }

    /**
     * Get the rank of this mdarray.
     * 
     * @return size_type 
     */
    size_type rank() const noexcept
    {
        return this->m_shape.size();
    }

    /**
     * Get the shape of this mdarray.
     * 
     * @return span of the extents per dimension
     */
    boost::span<const size_t> shape() const noexcept
    {
        return this->m_shape;
    }

    /**
     * Index a multi-dimensional set of indices to a single address index.
     * 
     * @param n 
     * @return size_type 
     */
    size_type index(const boost::span<const size_type> n) const
    {
        size_type index = 0, stride = 1;
        for (size_type k = 0; k < this->rank(); k++) {
            assert(n[k] < m_shape[k]);
            index += n[k] * stride;
            stride *= this->m_shape[k];
        }

        assert(index < this->size());
    
        return index;
    }

    /**
     * Retrieve the multi-dimensional index from a given address index.
     * 
     * @param[in] idx Address index (aka flat index)
     * @param[out] n outputted index
     */
    void deindex(size_type idx, boost::span<size_type> n) const
    {
        assert(n.size() == this->rank());
        assert(idx < this->size());

        size_type stride = 1;
        for (size_type k = 0; k < this->rank(); k++) {
            n[k] = idx / stride % this->m_shape[k];
            stride *= this->m_shape[k];
        }
    }

  private:

    inline void bounds_check(const boost::span<const size_type> n) const
    {
        // This bounds check is required since `index` performs
        // modulo arithmetic on the dimension indices with carryover,
        // such that if an mdarray has shape {2, 2}, the index {2, 0}
        // is equivalent to {0, 1}.
        for (size_type i = 0; i < this->m_shape.size(); i++) {
            if (n[i] >= this->m_shape[i])
                throw std::out_of_range(
                    "index " + std::to_string(n[i]) +
                    " must be less than dimension size " +
                    std::to_string(this->m_shape[i])
                );
        }
    }
    
    std::vector<size_type> m_shape;
    container_type         m_data;
};

} // namespace ngen

#endif // NGEN_MDARRAY_DEFINITION_HPP
