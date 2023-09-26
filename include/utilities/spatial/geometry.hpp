#ifndef NGEN_UTILITIES_SPATIAL_GEOMETRY_GEOMETRY_HPP
#define NGEN_UTILITIES_SPATIAL_GEOMETRY_GEOMETRY_HPP

#include <memory>

#include "traits.hpp"

namespace ngen {
namespace spatial {

enum class geometry_type
{
    point,
    linestring,
    polygon,
    multipoint,
    multilinestring,
    multipolygon,
    geometry_collection
};

struct geometry
{
    using size_type          = std::size_t;
    using pointer            = std::shared_ptr<geometry>;
    using const_pointer      = std::shared_ptr<const geometry>;
    using weak_pointer       = std::weak_ptr<geometry>;
    using const_weak_pointer = std::weak_ptr<const geometry>;

    geometry()                           = delete;
    geometry(const geometry&)            = delete;
    geometry& operator=(const geometry&) = delete;
    geometry(geometry&&)                 = delete;
    geometry& operator=(geometry&&)      = delete;

    virtual ~geometry() noexcept = 0;
};

template<typename BackendPolicy>
struct point : public virtual geometry
{
    using geometry::geometry;
    ~point() noexcept override = 0;

    double x() const noexcept;
    double y() const noexcept;

  private:
    using traits = backend_traits<BackendPolicy>;
    using type   = typename traits::point_type;

    type data_;
};

template<typename BackendPolicy>
struct linestring : public virtual geometry
{
    using point_type = point<BackendPolicy>;

    using geometry::geometry;
    ~linestring() noexcept override = 0;

    virtual double      length()      const noexcept = 0;
    virtual point_type  start_point() const noexcept = 0;
    virtual point_type  end_point()   const noexcept = 0;
    virtual bool        is_closed()   const noexcept = 0;
    virtual bool        is_ring()     const noexcept = 0;
    virtual int         size()        const noexcept = 0;
    virtual point_type  at(int n)     const noexcept = 0;
};

struct polygon : public virtual geometry
{
    using geometry::geometry;
    ~polygon() noexcept override = 0;
};

} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_GEOMETRY_HPP
