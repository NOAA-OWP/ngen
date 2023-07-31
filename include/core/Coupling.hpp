#pragma once

#include "ModelVariable.hpp"

#include <memory>

struct Coupling
{
    ~Coupling() = default;
    
    // Read values from connected input, and write (possibly converted,
    // interpolated, scaled, etc) values to connected output
    virtual void transfer() = 0;
};

struct DirectCoupling : public Coupling
{
    DirectCoupling(shared_ptr<SourceVariable> from, shared_ptr<SinkVariable> to)
        : from_(from)
        , to_(to)
    {
        assert(from->discretization()->equals(to->discretization().get()));

        assert(from->element_dimensionality() == to->element_dimensionality());
        // TODO: relax this to automatic unit conversion by scaling
        assert(from->element_units() == to->element_units());
    }

    DirectCoupling(shared_ptr<SourceVariable> from, shared_ptr<SinkVariable> to,
                   double scaling)
        : from_(from)
        , to_(to)
        , scaling_(scaling)
    { 
        assert(from->discretization()->equals(to->discretization().get()));

        // Don't assert that dimensionality/units are common - the
        // scaling should be set to address that
    }

    void transfer() override
    {
        // Short-circuit the case of shared storage
        if (dynamic_pointer_cast<ModelVariable>(from_) == dynamic_pointer_cast<ModelVariable>(to_))
            return;

        boost::span<double> src = from_->storage(), dst = to_->storage();
        for (size_t i = 0; i < src.size(); ++i) {
            dst[i] = scaling_ * src[i];
        }
    }

private:
    shared_ptr<SourceVariable> from_;
    shared_ptr<SinkVariable> to_;
    double scaling_ = 1.0;
};

struct LumpedPrecipitationRateCoupling : public DirectCoupling
{
    constexpr static double unit_correction = 1.0; // FIXME
    LumpedPrecipitationRateCoupling(shared_ptr<SourceVariable> from,
                              shared_ptr<SinkVariable> to,
                              std::chrono::duration<double> timestep,
                              double area_sqkm)
        : DirectCoupling(from, to, timestep.count()*area_sqkm*unit_correction)
    {
        //assert(from->element_dimensionality() == length/time);
        //assert(to->element_dimensionality() == length*length*length);
    }
};

template <typename T>
using SparseMatrix = int;

// Can handle subsetting, permutation, linear interpolation,
// (potentially conservative) partitioning, and other conversions that
// involve only linear combinations of the input to produce the output
struct LinearTransferCoupling : public Coupling
{
    LinearTransferCoupling(shared_ptr<SourceVariable> from,
                           shared_ptr<SinkVariable> to,
                           SparseMatrix<double> transfer_weights)
        : from_(from)
        , to_(to)
        , transfer_weights_(transfer_weights)
    { }

    void transfer() override
    {
        //SpMV(transfer_weights_(), from_->storage(), to_->storage());
    }

    shared_ptr<SourceVariable> from_;
    shared_ptr<SinkVariable> to_;
    SparseMatrix<double> transfer_weights_;
};
