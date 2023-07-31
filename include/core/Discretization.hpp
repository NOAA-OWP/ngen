#pragma once

#include "span.hpp"

using GeoDivide = int;

struct Discretization
{
    virtual ~Discretization() = default;

    enum GridType { hydrofabric_locations, spatial_structured, spatial_unstructured };
    virtual GridType grid_type() = 0;

    // Must cover an identified geographic domain
    virtual GeoDivide divide() = 0;

    virtual size_t size() = 0;
    virtual int rank() = 0;

    virtual bool equals(Discretization const* const rhs) = 0;
};

template <typename T>
struct ConcreteDiscretization : public Discretization
{
    using PointType = T;
    virtual boost::span<PointType> get_points() = 0;
};

// One point per catchment/nexus/whatever set elements
struct HydroLocationDiscretization : public ConcreteDiscretization<int>
{
    GridType grid_type() override { return hydrofabric_locations; }
    GeoDivide divide() override { return 0;
    }
    size_t size() override { return 1; }
    int rank() override { return 1; }
    boost::span<PointType> get_points() override {  return {}; }

    bool equals(Discretization const * const rhs) override {
        auto cast_rhs = dynamic_cast<HydroLocationDiscretization const*>(rhs);

        if (cast_rhs == nullptr)
            return false;

        return true;
    }
};

struct CartesianMeshDiscretization : public ConcreteDiscretization<float>
{
    // coordinate reference, origin, spacing, point count
    GridType grid_type() override { return spatial_structured; }
    GeoDivide divide() override { return 0; }
    size_t size() override { return 1; }
    int rank() override { return 2; }
    boost::span<PointType> get_points() override {  return {}; }

    bool equals(Discretization const * const rhs) override {
        auto cast_rhs = dynamic_cast<CartesianMeshDiscretization const*>(rhs);

        if (cast_rhs == nullptr)
            return false;

        return true;
    }
};

struct UnstructuredMeshDiscretization : public ConcreteDiscretization<char>
{
    // coordinate reference, point count, point coordinates
    GridType grid_type() override { return spatial_unstructured; }
    GeoDivide divide() override { return 0; }
    size_t size() override { return 1; }
    int rank() override { return 1; }
    boost::span<PointType> get_points() override {  return {}; }

    bool equals(Discretization const * const rhs) override {
        auto cast_rhs = dynamic_cast<UnstructuredMeshDiscretization const*>(rhs);

        if (cast_rhs == nullptr)
            return false;

        return true;
    }
};
