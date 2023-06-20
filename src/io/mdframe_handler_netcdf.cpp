#include "mdframe.hpp"

#include <netcdf>

namespace io {
namespace visitors {
    struct mdarray_putvar : boost::static_visitor<void>
    {
        template<typename T>
        void operator()(mdarray<T> v, netCDF::NcVar& var)
        {
            for (const mdvalue<T>& value : v) {
                auto index = value.decode(value.m_index, value.m_rank);
                var.putVar(index, value.m_value);
            }
        }
    };
}
}

void io::mdframe::to_netcdf(const std::string& path) const
{
    netCDF::NcFile output{path, netCDF::NcFile::replace};

    std::unordered_map<std::string, netCDF::NcDim> dimmap;
    std::unordered_map<std::string, netCDF::NcVar> varmap;

    for (const auto& dim : this->m_dimensions) {
        dimmap[dim.name()] = dim.size() == boost::none
            ? output.addDim(dim.name())
            : output.addDim(dim.name(), dim.size().get());
    }

    for (const auto& var : this->m_variables) {
        netCDF::NcType* type = nullptr;
        switch (var.values().which()) {
            case 0: // int
            case 2: // boolean
                type = &netCDF::ncInt;
                break;

            case 3: // string
                type = &netCDF::ncString;
                break;

            case 1: // double
            default:
                type = &netCDF::ncDouble;
                break;
        }

        // get list of dimensions
        std::vector<netCDF::NcDim> dimensions;
        dimensions.reserve(var.rank());
        for (const auto& dimname : var.dimensions()) {
            dimensions.push_back(dimmap[dimname]);
        }

        // Create netcdf var
        const auto& nc_var = output.addVar(var.name(), *type, dimensions);
        varmap[var.name()] = nc_var;

        // Put values into netcdf var
        auto visitor = std::bind(
            io::visitors::mdarray_putvar{},
            std::placeholders::_1,
            nc_var
        );

        var.values().apply_visitor(visitor);
    }
}