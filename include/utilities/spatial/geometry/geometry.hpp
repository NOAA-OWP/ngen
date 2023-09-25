#ifndef NGEN_UTILITIES_SPATIAL_GEOMETRY_GEOMETRY_HPP
#define NGEN_UTILITIES_SPATIAL_GEOMETRY_GEOMETRY_HPP

#include <memory>

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

namespace pm {

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

struct point : public virtual geometry
{
    using geometry::geometry;
    ~point() noexcept override = 0;

    virtual double x() const noexcept = 0;
    virtual double y() const noexcept = 0;
    virtual double z() const noexcept = 0;
    virtual double m() const noexcept = 0;
};

struct linestring : public virtual geometry
{
    using geometry::geometry;
    ~linestring() noexcept override = 0;

    virtual double length()      const noexcept = 0;
    virtual point  start_point() const noexcept = 0;
    virtual point  end_point()   const noexcept = 0;
    virtual bool   is_closed()   const noexcept = 0;
    virtual bool   is_ring()     const noexcept = 0;
    virtual int    size()        const noexcept = 0;
    virtual point  at(int n)     const noexcept = 0;
};

} // namespace pm
} // namespace spatial
} // namespace ngen

#endif // NGEN_UTILITIES_SPATIAL_GEOMETRY_GEOMETRY_HPP
