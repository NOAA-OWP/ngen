#ifndef NGEN_UTILITIES_PROPERTY_HPP
#define NGEN_UTILITIES_PROPERTY_HPP

#include <unordered_map>
#include <vector>
#include <cstdint>

#include <boost/variant.hpp>
#include <boost/type_traits.hpp>

#include "span.hpp"
#include "traits.hpp"

namespace ngen {

namespace detail {

template<typename R, typename... Fs>
struct functor;

template<typename R, typename F, typename... Fs>
struct functor<R, F, Fs...>
    : public functor<R, Fs...>
    , public F
{
    using typename boost::static_visitor<R>::result_type;

    using F::operator();
    using functor<R, Fs...>::operator();

    explicit functor(F f, Fs... fs)
      : F(f), functor<R, Fs...>(fs...){}
};

template<typename R, typename F>
struct functor<R, F> : public boost::static_visitor<R>, public F
{
    using typename boost::static_visitor<R>::result_type;

    using F::operator();
    explicit functor(F f)
      : boost::static_visitor<R>()
      , F(f){};
};

template<typename R>
struct functor<R> : public boost::static_visitor<R>
{
    using typename boost::static_visitor<R>::result_type;

    functor()
      : boost::static_visitor<R>(){};
};

template<typename R, typename... Fs>
auto visitor(Fs&&... fs)
{
    return detail::functor<R, std::decay_t<Fs>...>{std::forward<Fs>(fs)...};
}

} // namespace detail

struct property;

struct property_list
{
    using value_type      = property;
    using container_type  = std::vector<value_type>;
    using size_type       = typename container_type::size_type;
    using reference       = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using iterator        = typename container_type::iterator;
    using const_iterator  = typename container_type::const_iterator;

    explicit property_list(container_type&& values);
    explicit property_list(boost::span<property> values);

    template<typename Tp>
    std::vector<Tp> cast() const noexcept;

    template<typename Tp>
    Tp& cast_at(size_type n);

    template<typename Tp>
    const Tp& cast_at(size_type n) const;

    template<typename... Args>
    void emplace_back(Args&&... args);

    void            push_back(const_reference value);
    size_type       size() const noexcept;
    reference       at(size_type n);
    const_reference at(size_type n) const;
    reference       operator[](size_type n) noexcept;
    const_reference operator[](size_type n) const noexcept;
    iterator        begin() noexcept;
    iterator        end() noexcept;
    const_iterator  begin() const noexcept;
    const_iterator  end() const noexcept;

    friend std::ostream& operator<<(std::ostream& stream, const property_list& p);

  private:
    container_type data_;
};

struct property_map
{
    using key_type        = std::string;
    using value_type      = property;
    using container_type  = std::unordered_map<key_type, value_type>;
    using size_type       = typename container_type::size_type;
    using reference       = typename container_type::reference;
    using const_reference = typename container_type::const_reference;
    using iterator        = typename container_type::iterator;
    using const_iterator  = typename container_type::const_iterator;

    explicit property_map(container_type&& values);
    explicit property_map(const container_type&& values);

    template<typename Tp>
    std::unordered_map<std::string, Tp> cast() const noexcept;

    template<typename Tp>
    Tp& cast_at(const key_type& key);

    template<typename Tp>
    const Tp& cast_at(const key_type& key) const;

    template<typename... Args>
    void emplace_back(key_type key, Args&&... args);

    void            push_back(const_reference value);
    size_type       size() const noexcept;
    reference       at(const key_type& n);
    const_reference at(const key_type& n) const;
    reference       operator[](const key_type& n) noexcept;
    const_reference operator[](const key_type& n) const noexcept;
    iterator        begin() noexcept;
    iterator        end() noexcept;
    const_iterator  begin() const noexcept;
    const_iterator  end() const noexcept;

    friend std::ostream& operator<<(std::ostream& stream, const property_map& p);

  private:
    container_type data_;
};

using property_types = traits::type_list<
    boost::blank,
    std::int64_t,
    double,
    bool,
    std::string,
    boost::recursive_wrapper<property_list>,
    boost::recursive_wrapper<property_map>
>;

class property
{
    using base_type = property_types::variant_scalar;

    template<typename... Args>
    explicit property(Args&&... args)
      : base_type(std::forward<Args>(args)...){};

    template<typename Tp>
    Tp& get()
    {
        return boost::get<Tp>(*this);
    }

    template<typename Tp>
    const Tp& get() const
    {
        return boost::get<Tp>(*this);
    }

    template<typename R, typename... Fs>
    auto visit(Fs&&... visitors)
    {
        return boost::apply_visitor(detail::visitor<R>(std::forward<Fs>(visitors)...), data_);
    }

    friend std::ostream& operator<<(std::ostream& stream, property& p) {
        p.visit<void>(
            [&](boost::blank){ stream << "<blank>"; },
            [&](const auto& x){ stream << x; }
        );
        return stream;
    }

  private:
    std::string key_;
    base_type data_;
};

} // namespace ngen

#endif // NGEN_UTILITIES_PROPERTY_HPP
