#include "mdframe.hpp"

#include <netcdf>

namespace io {

namespace visitors { // -------------------------------------------------------

/**
 * mdarray visitor for putting values from the mdarray into a netCDF var.
 */
MDARRAY_VISITOR(mdarray_putvar, void)
{
    MDARRAY_VISITOR_TEMPLATE_IMPL(T& v, netCDF::NcVar& var) -> void
    {
        for (auto it = v.begin(); it != v.end(); it++) {
            var.putVar(it.mdindex(), *it);
        }
    }
};

} // namespace visitors -------------------------------------------------------

} // namespace io

void io::mdframe::to_netcdf(const std::string& path) const
{
    netCDF::NcFile output{path, netCDF::NcFile::replace};

    std::unordered_map<std::string, netCDF::NcDim> dimmap;
    std::unordered_map<std::string, netCDF::NcVar> varmap;

    for (const auto& dim : this->m_dimensions) {
        dimmap[dim.name()] = output.addDim(dim.name(), dim.size());
    }

    for (const auto& pair : this->m_variables) {
        netCDF::NcType* type = nullptr;
        const auto var = pair.second;
        switch (var.values().which()) {
            case 0: // int
                type = &netCDF::ncInt;
                break;

            case 1: // float
                type = &netCDF::ncFloat;
                break;
        
            case 2: // double
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