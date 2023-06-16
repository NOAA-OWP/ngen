#ifndef NGEN_IO_MDARRAY_HPP
#define NGEN_IO_MDARRAY_HPP

#include <set>
#include <stdexcept>

#include "mdvalue.hpp"

namespace io {

#warning NO DOCUMENTATION
template<typename T>
class mdarray {
  public:
    using value_type      = T;
    using element_type    = mdvalue<value_type>;
    using index_type      = typename element_type::ilist_type;
    using container       = std::set<element_type>;
    using size_type       = typename container::size_type;
    using difference_type = typename container::difference_type;
    using reference       = typename container::reference;
    using const_reference = typename container::const_reference;
    using pointer         = typename container::pointer;
    using const_pointer   = typename container::const_pointer;
    using iterator        = typename container::iterator;
    using const_iterator  = typename container::const_iterator;

    // Default constructor
    mdarray() = default;

    /**
     * Construct an empty mdarray with the
     * specified rank
     * 
     * @param rank Number of dimensions
     */
    mdarray(size_type rank) noexcept
        : m_data()
        , m_rank(rank){};

    // ========================================================================
    // STL Interface
    // ========================================================================

    // Iterators --------------------------------------------------------------
    #warning NO DOCUMENTATION
    iterator       begin() noexcept { return this->m_data.begin(); }

    #warning NO DOCUMENTATION
    const_iterator begin() const noexcept { return this->m_data.begin(); }

    #warning NO DOCUMENTATION
    iterator       end() noexcept { return this->m_data.end(); }

    #warning NO DOCUMENTATION
    const_iterator end() const noexcept { return this->m_data.end(); }

    // Capacity ---------------------------------------------------------------
    #warning NO DOCUMENTATION
    bool empty() const noexcept { return this->m_data.empty(); }

    #warning NO DOCUMENTATION
    size_type size() const noexcept { return this->m_data.size(); }

    #warning NO DOCUMENTATION
    size_type max_size() const noexcept { return this->m_data.max_size(); }

    // Modifiers --------------------------------------------------------------
    #warning NO DOCUMENTATION
    void clear() noexcept
    {
        this->m_data.clear();
    }

    #warning NO DOCUMENTATION
    std::pair<iterator, bool> emplace(std::initializer_list<size_type> index, value_type value)
    {
        return this->m_data.emplace(index, value);
    }

    #warning NO DOCUMENTATION
    void emplace(std::initializer_list<std::pair<std::initializer_list<size_type>, value_type>> elements)
    {
        for (const auto& e : elements) {
          this->emplace(e.first, e.second);
        }
    }

    #warning NO DOCUMENTATION
    std::pair<iterator, bool> insert(const element_type& value)
    {
        return this->m_data.insert(value);
    }

    #warning NO DOCUMENTATION
    std::pair<iterator, bool> insert(element_type&& value)
    {
        return this->m_data.insert(std::move(value));
    }

    #warning NO DOCUMENTATION
    void insert(std::initializer_list<element_type> values)
    {
        this->m_data.insert(values);
    }

    // Lookup -----------------------------------------------------------------
    #warning NO DOCUMENTATION
    const_reference at(std::initializer_list<size_type> n)
    {
        iterator x = this->m_data.find(n);
        if (x == this->m_data.end())
          #warning NO ERROR MESSAGE
          throw std::out_of_range("");
      
        return *x;
    }

    #warning NO DOCUMENTATION
    const_reference at(std::initializer_list<size_type> n) const
    {
        const_iterator x = this->m_data.find(n);
        if (x == this->m_data.end())
          #warning NO ERROR MESSAGE
          throw std::out_of_range("");
      
        return *x;
    }

    // ========================================================================
    // mdarray-specific Interface
    // ========================================================================
    #warning NO DOCUMENTATION
    size_type rank() const noexcept
    {
        return this->m_rank;
    }

    #warning NO DOCUMENTATION
    void rank(size_type r) noexcept
    {
        this->m_rank = r;
    }

  private:
    container m_data;
    size_type m_rank;
};

} // namespace io

#endif // NGEN_IO_MDARRAY_HPP
