#ifndef NGEN_UTILITIES_PROPERTY_HPP
#define NGEN_UTILITIES_PROPERTY_HPP

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include <boost/variant.hpp>
#include <boost/type_traits.hpp>

#include "span.hpp"
#include "traits.hpp"

namespace ngen {

namespace detail {

//! Lambda-wrapper that is applicative as
//! a boost::static_visitor.
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
      : F(f)
      , functor<R, Fs...>(fs...)
    {}
};

template<typename R, typename F>
struct functor<R, F>
  : public boost::static_visitor<R>
  , public F
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
    return detail::functor<R, std::decay_t<Fs>...>{ std::forward<Fs>(fs)... };
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

    explicit property_list(container_type&& values)
      : data_(std::move(values)){};
    
    template<typename InputIterator>
    explicit property_list(InputIterator begin, InputIterator end)
      : data_(begin, end){};

    template<typename Tp>
    std::vector<Tp> cast() const noexcept;

    template<typename Tp>
    Tp& cast_at(size_type n);

    template<typename Tp>
    const Tp& cast_at(size_type n) const;

    template<typename... Args>
    void emplace_back(Args&&... args)
    {
        data_.emplace_back(std::forward<Args>(args)...);
    }

    void push_back(const value_type& value)
    {
        data_.push_back(value);
    }

    void push_back(value_type&& value)
    {
        data_.push_back(std::move(value));
    }

    size_type size() const noexcept
    {
        return data_.size();
    }

    reference at(size_type n)
    {
        return data_.at(n);
    }

    const_reference at(size_type n) const
    {
        return data_.at(n);
    }

    reference operator[](size_type n) noexcept
    {
        return data_[n];
    }
    
    const_reference operator[](size_type n) const noexcept
    {
        return data_[n];
    }

    iterator begin() noexcept
    {
        return data_.begin();
    }

    iterator end() noexcept
    {
        return data_.end();
    }

    const_iterator begin() const noexcept
    {
        return data_.begin();
    }

    const_iterator end() const noexcept
    {
        return data_.end();
    }

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
    using reference       = value_type&;
    using const_reference = const value_type&;
    using iterator        = typename container_type::iterator;
    using const_iterator  = typename container_type::const_iterator;

    explicit property_map(container_type&& values);
    explicit property_map(const container_type&& values);

    template<typename Tp>
    Tp& cast_at(const key_type& key);

    template<typename Tp>
    const Tp& cast_at(const key_type& key) const;

    template<typename... Args>
    void emplace(const key_type& key, Args&&... args);

    size_type size() const noexcept
    {
        return data_.size();
    }

    reference at(const key_type& key)
    {
        return data_.at(key);
    }

    const_reference at(const key_type& key) const
    {
        return data_.at(key);
    }

    reference operator[](const key_type& key) noexcept
    {
        return data_[key];
    }

    const_reference operator[](const key_type& key) const noexcept
    {
        return data_.at(key);
    }

    iterator begin() noexcept
    {
        return data_.begin();
    }

    iterator end() noexcept
    {
        return data_.end();
    }

    const_iterator begin() const noexcept
    {
        return data_.begin();
    }

    const_iterator end() const noexcept
    {
        return data_.end();
    }

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
  boost::recursive_wrapper<property_map>>;

struct property
{
    using base_type = property_types::variant_scalar;

    property()      = default;

    template<typename... Args>
    explicit property(Args&&... args)
      : data_(std::forward<Args>(args)...){};

    template<typename Tp, property_types::enable_if_supports<Tp, bool> = true>
    Tp& get()
    {
        return boost::get<Tp>(*this);
    }

    template<typename Tp, property_types::enable_if_supports<Tp, bool> = true>
    const Tp& get() const
    {
        return boost::get<Tp>(*this);
    }

    template<typename R, typename... Fs>
    auto visit(Fs&&... visitors)
    {
        return boost::apply_visitor(
          detail::visitor<R>(std::forward<Fs>(visitors)...), data_
        );
    }

    template<typename R, typename... Fs>
    auto visit(Fs&&... visitors) const
    {
        return boost::apply_visitor(detail::visitor<R>(std::forward<Fs>(visitors)...), data_);
    }

    friend std::ostream& operator<<(std::ostream& stream, const property& p) {
        p.visit<void>(
          [&](boost::blank) { stream << "<blank>"; }, [&](const auto& x) { stream << x; }
        );
        return stream;
    }

  private:
    std::string key_;
    base_type   data_;
};

template<typename Tp>
auto property_map::cast_at(const key_type& key)
  -> Tp&
{
    return at(key).get<Tp>();
}

template<typename Tp>
auto property_map::cast_at(const key_type& key) const
  -> const Tp&
{
    return at(key).get<Tp>();
}

template<typename... Args>
void property_map::emplace(const key_type& key, Args&&... args)
{
    data_.emplace(key, property{ std::forward<Args>(args)... });
}

template<typename Tp>
auto property_list::cast() const noexcept -> std::vector<Tp>
{
    std::vector<Tp> result;
    std::transform(data_.begin(), data_.end(), std::back_inserter(result), [](const property& p){
        return p.get<Tp>();
    });
    return result;
}

template<typename Tp>
auto property_list::cast_at(size_type n) -> Tp&
{
    return at(n).get<Tp>();
}

template<typename Tp>
auto property_list::cast_at(size_type n) const -> const Tp&
{
    return at(n).get<Tp>();
}

std::ostream& operator<<(std::ostream& stream, const property_map& p)
{
    std::stringstream output;
    output << "{";
    for (const auto& kv : p) {
        output << "\"" << kv.first << "\": " << kv.second << ",";
    }
    output.seekg(1, std::ios::end);
    output << "}";
    stream << output.str();
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const property_list& p)
{
    std::stringstream output;
    output << "[";
    for (const auto& v : p) {
        output << v << ", ";
    }
    output.seekg(1, std::ios::end);
    output << "]";
    stream << output.str();
    return stream;
}


} // namespace ngen

#endif // NGEN_UTILITIES_PROPERTY_HPP
