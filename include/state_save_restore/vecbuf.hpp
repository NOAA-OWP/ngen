#ifndef HPP_STRING_VECBUF
#define HPP_STRING_VECBUF
// https://gist.github.com/stephanlachnit/4a06f8475afd144e73235e2a2584b000
// SPDX-FileCopyrightText: 2023 Stephan Lachnit
// SPDX-License-Identifier: MIT

#include <streambuf>
#include <string>
#include <vector>

template<class CharT = char, class Traits = std::char_traits<CharT>>
class vecbuf : public std::basic_streambuf<CharT> {
public:
    using streambuf = std::basic_streambuf<CharT, Traits>;
    using char_type = typename streambuf::char_type;
    using int_type = typename streambuf::int_type;
    using traits_type = typename streambuf::traits_type;
    using vector = std::vector<char_type>;
    using value_type = typename vector::value_type;
    using size_type = typename vector::size_type;

    // Constructor for vecbuf with optional initial capacity
    vecbuf(size_type capacity = 0) : vector_() { reserve(capacity); }

    // Forwarder for std::vector::shrink_to_fit()
    constexpr void shrink_to_fit() { vector_.shrink_to_fit(); }
    
    // Forwarder for std::vector::clear()
    constexpr void clear() { vector_.clear(); }

    // Forwarder for std::vector::resize(size)
    constexpr void resize(size_type size) { vector_.resize(size); }

    // Forwarder for std::vector::reserve
    constexpr void reserve(size_type capacity) { vector_.reserve(capacity); setp_from_vector(); }

    // Increase the capacity of the buffer by reserving the current_size + additional_capacity
    constexpr void reserve_additional(size_type additional_capacity) { reserve(size() + additional_capacity); }

    // Forwarder for std::vector::data
    constexpr value_type* data() { return vector_.data(); }

    // Forwarder for std::vector::size
    constexpr size_type size() const { return vector_.size(); }

    // Forwarder for std::vector::capacity
    constexpr size_type capacity() const { return vector_.capacity(); }

    // Implements std::basic_streambuf::xsputn
    std::streamsize xsputn(const char_type* s, std::streamsize count) override {
        try {
            reserve_additional(count);
        }
        catch (const std::bad_alloc& error) {
            // reserve did not work, use slow algorithm
            return xsputn_slow(s, count);
        }
        // reserve worked, use fast algorithm
        return xsputn_fast(s, count);
    }

protected:
    // Calculates value to std::basic_streambuf::pbase from vector
    constexpr value_type* pbase_from_vector() const { return const_cast<value_type*>(vector_.data()); }

    // Calculates value to std::basic_streambuf::pptr from vector
    constexpr value_type* pptr_from_vector() const { return const_cast<value_type*>(vector_.data() + vector_.size()); }

    // Calculates value to std::basic_streambuf::epptr from vector
    constexpr value_type* epptr_from_vector() const { return const_cast<value_type*>(vector_.data()) + vector_.capacity(); }

    // Sets the values for std::basic_streambuf::pbase, std::basic_streambuf::pptr and std::basic_streambuf::epptr from vector
    constexpr void setp_from_vector() { streambuf::setp(pbase_from_vector(), epptr_from_vector()); streambuf::pbump(size()); }

private:
    // std::vector containing the data
    vector vector_;

    // Fast implementation of std::basic_streambuf::xsputn if reserve_additional(count) succeeded
    std::streamsize xsputn_fast(const char_type* s, std::streamsize count) {
        // store current pptr (end of vector location)
        auto* old_pptr = pptr_from_vector();
        // resize the vector, does not move since space already reserved
        vector_.resize(vector_.size() + count);
        // directly memcpy new content to old pptr (end of vector before it was resized)
        traits_type::copy(old_pptr, s, count);
        // reserve() already calls setp_from_vector(), only adjust pptr to new epptr
        streambuf::pbump(count);

        return count;
    }

    // Slow implementation of std::basic_streambuf::xsputn if reserve_additional(count) did not succeed, might calls std::basic_streambuf::overflow()
    std::streamsize xsputn_slow(const char_type* s, std::streamsize count) {
        // reserving entire vector failed, emplace char for char
        std::streamsize written = 0;
        while (written < count) {
            try {
                // copy one char, should throw eventually std::bad_alloc
                vector_.emplace_back(s[written]);
            }
            catch (const std::bad_alloc& error) {
                // try overflow(), if eof return, else continue
                int_type c = this->overflow(traits_type::to_int_type(s[written]));
                if (traits_type::eq_int_type(c, traits_type::eof())) {
                    return written;
                }
            }
            // update pbase, pptr and epptr
            setp_from_vector();
            written++;
        }
        return written;
    }

};

class membuf : public std::streambuf {
public:
    membuf(char *begin, size_t size) {
        this->setg(begin, begin, begin + size);
    }
};

#endif
