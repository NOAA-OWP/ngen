#pragma once

#include <forcing/DataProvider.hpp>
#include <forcing/MeshPointsSelectors.hpp>

#include <string>
#include <boost/core/span.hpp>

class CoastalFormulation : public data_access::DataProvider<double, MeshPointsSelector>
{
public:
    CoastalFormulation(std::string id)
        : id_(std::move(id))
    { }

    virtual ~CoastalFormulation() = default;

    std::string get_id() const {
        return this->id_;
    }

    virtual void initialize() = 0;
    virtual void finalize() = 0;
    virtual void update() = 0;

    // The interface that DataProvider really should have
    virtual void get_values(const selection_type& selector, boost::span<double> data) = 0;

    // And an implementation of the usual version using it
    std::vector<data_type> get_values(const selection_type& selector, data_access::ReSampleMethod) override
    {
        std::vector<data_type> output(selected_points_count(selector));
        get_values(selector, output);
        return output;
    }

protected:
    using selection_type = MeshPointsSelector;
    using data_type = double;

    virtual size_t mesh_size(std::string const& variable_name) = 0;

    size_t selected_points_count(const selection_type& selector)
    {
        auto* points = boost::get<std::vector<int>>(selector.points);
        size_t size = points ? points->size() : this->mesh_size(selector.variable_name);
        return size;
    }

private:
    const std::string id_;
};
