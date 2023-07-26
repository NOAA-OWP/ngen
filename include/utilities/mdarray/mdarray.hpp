#ifndef NGEN_MDARRAY_DEFINITION_HPP
#define NGEN_MDARRAY_DEFINITION_HPP

#include <vector>

#include <span.hpp>

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

    // Deleted default constructor. mdarray must be given a rank at least.
    mdarray() = delete;

    mdarray(size_type rank) noexcept
        : m_shape(rank)
        , m_data() {};

    mdarray(boost::span<size_t> dsizes)
        : m_shape(dsizes.begin(), dsizes.end())
        , m_data(this->max_size()) {};

    mdarray(std::initializer_list<size_t> dsizes)
        : m_shape(dsizes.begin(), dsizes.end())
        , m_data(this->max_size()) {};

    /**
     * Retrieve a reference to the value at the given index.
     *
     * @example
     * mdarray<int> x(2); // rank 2
     * x.at({0, 0});      // get the value at (0, 0) in a 2D array.
     * x.at({1, 1}) = 3;  // set the value at (1, 1) in a 2D array.
     *
     * @param n Index list
     * @return reference 
     */
    reference at(const boost::span<const size_t> n)
    {
        return this->m_data.at(this->index(n));
    }

    /**
     * Retrieve const reference to the value at the given index.
     *
     * @example
     * mdarray<int> x(2); // rank 2
     * x.at({0, 0});      // get the value at (0, 0) in a 2D array.
     *
     * @param n Index list
     * @return reference 
     */
    const_reference at(const boost::span<const size_t> n) const
    {
        return this->m_data.at(this->index(n));
    }

    void insert(const boost::span<const size_t> n, value_type value)
    {
        for (size_type i = 0; i < this->m_shape.size(); i++) {
            if (n[i] >= this->m_shape[i])
                throw std::out_of_range("out of range");
        }

        size_type index = this->index(n);
    
        if (index > this->max_size())
            throw std::out_of_range("out of range");
        
        if (index > this->size() || this->size() == 0)
            this->m_data.resize(index + 1);
    
        this->m_data[index] = value;
    }

    void insert(std::initializer_list<std::pair<std::initializer_list<size_type>, value_type>> args)
    {
        for (const auto& arg : args) {
            this->insert(arg.first, arg.second);
        }
    }

    /**
     * Allocate this mdarray based on the indices given.
     * 
     * @example
     * mdarray<double> x(3);   // rank 3
     * x.allocate({5, 10, 10}) // Allocates 5 * 10 * 10 elements
     * @param n 
     */
    void allocate(const boost::span<const size_t> n)
    {
        this->m_data.resize(this->index(n));
    }

    /**
     * Get the (total) size of this mdarray
     * (aka the total number of elements).
     * 
     * @return size_type
     */
    size_type size() const noexcept
    {
        return this->m_data.size();
    }

    size_type max_size() const noexcept
    {
        size_type max = 1;
        for (const auto& v : this->m_shape) {
            max *= v;
        }
        return max;
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
    size_type index(const boost::span<const size_t> n) const
    {
        size_type index = 0, stride = 1;
        for (size_type k = 0; k < this->rank(); k++) {
            index += n[k] * stride;
            stride *= this->m_shape[k];
        }
    
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
        // TODO: Need a bounds check?
        size_type stride = 1;
        for (size_type k = 0; k < this->rank(); k++) {
            n[k] = idx / stride % this->m_shape[k];
            stride *= this->m_shape[k];
        }
    }

  private:
    std::vector<size_type> m_shape;
    container_type         m_data;
};

} // namespace ngen

#endif // NGEN_MDARRAY_DEFINITION_HPP
