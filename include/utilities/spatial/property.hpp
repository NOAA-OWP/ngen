#ifndef NGEN_UTILITIES_SPATIAL_PROPERTY_HPP
#define NGEN_UTILITIES_SPATIAL_PROPERTY_HPP

#include <boost/variant.hpp>

namespace ngen {
namespace spatial {

namespace detail {

template<typename... Fs>
struct functor;

template<typename F, typename... Fs>
struct functor<F, Fs...>
    : public functor<Fs...>
    , public F
{
    using F::operator();
    using functor<Fs...>::operator();

    explicit functor(F f, Fs... fs)
      : F(f), functor<Fs...>(fs...){}
};

template<typename F>
struct functor<F> : public F
{
    using F::operator();
    explicit functor(F f)
      : F(f){};
};

} // namespace detail

template<typename... Fs>
auto visitor(Fs&&... fs)
{
    return detail::functor<std::decay_t<Fs>...>{std::forward<Fs>(fs)...};
}

struct property : boost::variant<int, double, bool, std::string>
{
    using base_type = boost::variant<int, double, bool, std::string>;

    using base_type::variant;
    using base_type::operator!=;
    using base_type::operator<;
    using base_type::operator<=;
    using base_type::operator=;
    using base_type::operator==;
    using base_type::operator>;
    using base_type::operator>=;

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

    template<typename Visitor>
    auto visit(Visitor& visitor) const
    {
        return boost::apply_visitor(visitor, *this);
    }

    template<typename... Fs>
    auto visit(Fs&&... visitors) const
    {
        return boost::apply_visitor(visitor(std::forward<Fs>(visitors)...), *this);
    }

    friend std::ostream& operator<<(std::ostream& stream, const property& p) {
        p.visit([&](auto x){ stream << x; });
        return stream;
    }
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_PROPERTY_HPP
