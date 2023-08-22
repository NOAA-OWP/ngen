#ifndef NGEN_MDARRAY_ITERATOR_HPP
#define NGEN_MDARRAY_ITERATOR_HPP

#include "mdarray.hpp"

namespace ngen {

template<typename T>
struct mdarray<T>::iterator
{
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = mdarray::value_type;
    using pointer           = mdarray::pointer;
    using reference         = mdarray::reference;
    using const_reference   = mdarray::const_reference;

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

    void mdindex(boost::span<size_type> n) const noexcept
    {
        return this->m_ref.deindex(this->m_idx, n);
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

};

}

#endif // NGEN_MDARRAY_ITERATOR_HPP
