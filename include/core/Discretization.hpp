#pragma once

#include "span.hpp"

using GeoDivide = int;

struct Discretization
{
    virtual ~Discretization() = default;

    // Aligned with the possible results from BMI's get_grid_type
    enum class GridType {
        scalar,
        points,
        vector,
        unstructured,
        structured_quadrilateral,
        rectilinear,
        uniform_rectilinear
    };
    virtual GridType grid_type() const = 0;

    // Must cover an identified geographic domain
    virtual GeoDivide divide() const = 0;

    virtual size_t size() const = 0;
    virtual int rank() const = 0;

    virtual bool equals(Discretization const* const rhs) const = 0;
};

template <typename T>
struct ConcreteDiscretization : public Discretization
{
    using PointType = T;
    virtual boost::span<PointType> get_points() const = 0;
};

struct ScalarDiscretization : public ConcreteDiscretization<std::nullptr_t>
{
    GridType grid_type() const override { return GridType::scalar; }
    GeoDivide divide() const override { return 0; }
    size_t size() const override { return 1; }
    int rank() const override { return 1; }
    boost::span<std::nullptr_t> get_points() const override { throw std::logic_error(""); }

    bool equals(Discretization const * const rhs) const override {
        auto cast_rhs = dynamic_cast<ScalarDiscretization const*>(rhs);

        if (cast_rhs == nullptr)
            return false;

        return true;
    }
};

struct VectorDiscretization : public ConcreteDiscretization<std::nullptr_t>
{
    GridType grid_type() const override { return GridType::vector; }
    GeoDivide divide() const override { return 0; }
    size_t size() const override { return n_; }
    int rank() const override { return 1; }
    boost::span<std::nullptr_t> get_points() const override { throw std::logic_error(""); }

    bool equals(Discretization const * const rhs) const override {
        auto cast_rhs = dynamic_cast<VectorDiscretization const*>(rhs);

        if (cast_rhs == nullptr)
            return false;

        return true;
    }

private:
    size_t n_;
};

// One point per catchment/nexus/whatever set elements
struct PointDiscretization : public ConcreteDiscretization<int>
{
    GridType grid_type() const override { return GridType::points; }
    GeoDivide divide() const override { return 0;
    }
    size_t size() const override { return 1; }
    int rank() const override { return 1; }
    boost::span<PointType> get_points() const override {  return {}; }

    bool equals(Discretization const * const rhs) const override {
        auto cast_rhs = dynamic_cast<PointDiscretization const*>(rhs);

        if (cast_rhs == nullptr)
            return false;

        return true;
    }
};

struct CartesianMeshDiscretization : public ConcreteDiscretization<float>
{
    // coordinate reference, origin, spacing, point count
    GridType grid_type() const override { return GridType::structured_quadrilateral; }
    GeoDivide divide() const override { return 0; }
    size_t size() const override { return 1; }
    int rank() const override { return 2; }
    boost::span<PointType> get_points() const override {  return {}; }

    bool equals(Discretization const * const rhs) const override {
        auto cast_rhs = dynamic_cast<CartesianMeshDiscretization const*>(rhs);

        if (cast_rhs == nullptr)
            return false;

        return true;
    }
};

struct UnstructuredDiscretization : public ConcreteDiscretization<char>
{
    // coordinate reference, point count, point coordinates
    GridType grid_type() const override { return GridType::unstructured; }
    GeoDivide divide() const override { return 0; }
    size_t size() const override { return 1; }
    int rank() const override { return 1; }
    boost::span<PointType> get_points() const override {  return {}; }

    bool equals(Discretization const * const rhs) const override {
        auto cast_rhs = dynamic_cast<UnstructuredDiscretization const*>(rhs);

        if (cast_rhs == nullptr)
            return false;

        return true;
    }
};
