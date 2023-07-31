#pragma once

#include "Discretization.hpp"
#include "realizations/catchment/Bmi_Formulation.hpp"
#include "forcing/GenericDataProvider.hpp"

using udunits_dimension_t = int;
using udunits_units_t = int;

struct ModelVariable
{
    virtual ~ModelVariable() = default;

    virtual shared_ptr<Discretization> discretization() const = 0;

    size_t size() const { return this->discretization()->size(); }
    int rank() const { return this->discretization()->rank(); }

    udunits_dimension_t element_dimensionality();
    udunits_units_t element_units();

    virtual boost::span<double> storage() const = 0;

    std::string name() { return name_; }

private:
    std::string name_;
};

struct BmiModelVariable : public virtual ModelVariable
{
    realization::Bmi_Formulation *model_;
};

struct SourceVariable : public virtual ModelVariable
{
    shared_ptr<data_access::GenericDataProvider> provider;

};


struct BmiSourceVariable : public BmiModelVariable, public SourceVariable
{
    // constraint: dynamic_cast<Bmi_Formulation*>(provider) != nullptr
};


struct SinkVariable : public virtual ModelVariable
{
};

struct BmiSinkVariable : public BmiModelVariable, public SinkVariable
{
};

struct MDFrameOutputVariable : public SinkVariable
{

};
