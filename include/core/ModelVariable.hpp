#pragma once

#include "Discretization.hpp"
#include "realizations/catchment/Bmi_Formulation.hpp"
#include "forcing/GenericDataProvider.hpp"

using udunits_dimension_t = int;
using udunits_units_t = int;

struct ModelVariable
{
    ModelVariable(std::string name)
        : name_{name}
    { }
    virtual ~ModelVariable() = default;

    virtual std::shared_ptr<Discretization const> discretization() const = 0;

    size_t size() const { return this->discretization()->size(); }
    int rank() const { return this->discretization()->rank(); }

    udunits_dimension_t element_dimensionality();
    udunits_units_t element_units();

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
    virtual boost::span<const double> storage() const = 0;


};

std::shared_ptr<Discretization const> const constant_discretization = std::make_shared<ScalarDiscretization>();

template <typename T>
struct ConstantVariable : public SourceVariable
{
    ConstantVariable(std::string name, T t)
        : ModelVariable{name}
        , t_{t}
    { }

    std::shared_ptr<Discretization const> discretization() const override {
        return constant_discretization;
    }

    boost::span<const double> storage() const override {
        return boost::span<const double>(&t_, 1);
    }

private:
    T const t_;
};

ConstantVariable<double> my_const{"MyConst", 1.3};

struct BmiSourceVariable : public BmiModelVariable, public SourceVariable
{
    shared_ptr<data_access::GenericDataProvider> provider;
    // constraint: dynamic_cast<Bmi_Formulation*>(provider) != nullptr
};


struct SinkVariable : public virtual ModelVariable
{
        virtual boost::span<double> storage() const = 0;
};

struct BmiSinkVariable : public BmiModelVariable, public SinkVariable
{
};

struct MDFrameOutputVariable : public SinkVariable
{

};
