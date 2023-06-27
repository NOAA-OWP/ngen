#ifndef NGEN_IO_MDARRAY_HPP
#define NGEN_IO_MDARRAY_HPP

#include <initializer_list>
#include <stdexcept>
#include <vector>

namespace io {

template<typename T>
class mdarray
{
  public:
    using value_type      = T;
    using container_type  = std::vector<value_type>;
    using ilist           = std::vector<std::size_t>;
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

    mdarray(ilist dsizes)
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
    reference at(const ilist& n)
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
    const_reference at(const ilist& n) const
    {
        return this->m_data.at(this->index(n));
    }

    void insert(const ilist& n, value_type value)
    {
        size_type index = this->index(n);
    
        if (index > this->max_size())
            throw std::out_of_range("out of range");
        
        if (index > this->size() || this->size() == 0)
            this->m_data.resize(index + 1);
    
        this->m_data.at(index) = value;
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
    void allocate(const ilist& n)
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
     * @return ilist
     */
    ilist shape() const noexcept
    {
        return this->m_shape;
    }

    /**
     * Index a multi-dimensional set of indices to a single address index.
     * 
     * @param n 
     * @return size_type 
     */
    size_type index(const ilist& n) const
    {
        size_type index = 0;
    
        for (size_type k = 0; k < this->rank(); k++) {
            size_type N = 1;
        
            for (size_type l = k + 1; l < this->rank(); l++) {
                N *= this->m_shape[l];
            }

            index += n[k] * N;
        }
    
        return index;
    }

    /**
     * Retrieve the multi-dimensional index from a given address index.
     * 
     * @param idx Address index (aka flat index)
     * @return ilist 
     */
    ilist deindex(size_type idx) const
    {
        if (idx == 0) { // trivial case
            return ilist(this->rank(), 0);
        }

        ilist n(this->rank());
    
        size_type addr = idx;

        for (size_type k = this->rank() - 1; k > 0; k--) {
            n[k] = addr % this->m_shape[k];
            addr /= this->m_shape[k];
        }

        n[0] = addr;

        return n;
    }

  private:
    ilist          m_shape;
    container_type m_data;
};

template<typename T>
struct mdarray<T>::iterator // ------------------------------------------------
{
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = mdarray::value_type;
    using pointer           = mdarray::pointer;
    using reference         = mdarray::reference;

    iterator(const mdarray& ref, size_type idx)
        : m_ref(ref)
        , m_idx(idx){};

    const_reference operator*() const
    {
        return this->m_ref.m_data.at(this->m_idx);
    }

    pointer operator->()
    {
        return &this->m_ref.m_data.at(this->m_idx);
    }

    iterator& operator++()
    {
        this->m_idx++;
        return *this;
    }

    iterator operator++(int)
    {
        iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    iterator& operator--()
    {
        this->m_idx--;
        return *this;
    }

    iterator operator--(int)
    {
        iterator tmp = *this;
        --(*this);
        return tmp;
    }

    ilist mdindex() const noexcept
    {
        return this->m_ref.deindex(this->m_idx);
    }

    friend bool operator==(const iterator& a, const iterator& b)
    {
        return (&a.m_ref == &b.m_ref) &&
                (a.m_idx == b.m_idx);
    }

    friend bool operator!=(const iterator& a, const iterator& b)
    {
        return !(a == b);
    }

    private:
    const mdarray& m_ref;
    size_type m_idx;

}; // struct iterator -----------------------------------------------------

} // namespace io

#endif // NGEN_IO_MDARRAY_HPP
