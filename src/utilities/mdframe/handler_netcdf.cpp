#include "mdframe/mdframe.hpp"

#include <NGenConfig.h>

#if NGEN_WITH_NETCDF

#include <netcdf>

namespace ngen {

namespace visitors {

struct mdarray_netcdf_putvar : public boost::static_visitor<void>
{

    template<typename T>
    void operator()(const mdarray<T>& arr, netCDF::NcVar& var)
    {
        typename mdarray<T>::size_type rank = arr.rank();
        std::vector<typename mdarray<T>::size_type> expanded_index(rank);
        
        for (auto it = arr.begin(); it != arr.end(); it++) {
            it.mdindex(expanded_index);
            var.putVar(expanded_index, *it);
        }
    }

};

} // namespace visitors

void mdframe::to_netcdf(const std::string& path) const
{
    netCDF::NcFile output{path, netCDF::NcFile::replace};

    std::unordered_map<std::string, netCDF::NcDim> dimmap;
    std::unordered_map<std::string, netCDF::NcVar> varmap;

    for (const auto& dim : this->m_dimensions)
        dimmap[dim.name()] = output.addDim(dim.name(), dim.size());

    for (const auto& pair : this->m_variables) {
        netCDF::NcType* type = nullptr;
        decltype(auto) var = pair.second;
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

        std::vector<netCDF::NcDim> dimensions;
        dimensions.reserve(var.rank());
        for (const auto& dimname : var.dimensions())
            dimensions.push_back(dimmap[dimname]);

        const auto& nc_var = output.addVar(var.name(), *type, dimensions);
        varmap[var.name()] = nc_var;

        auto visitor = std::bind(
            visitors::mdarray_netcdf_putvar{},
            std::placeholders::_1,
            nc_var
        );

        var.values().apply_visitor(visitor);
    }
}

} // namespace ngen

#else // NGEN_WITH_NETCDF

namespace ngen {
    void mdframe::to_netcdf(const std::string& path) const
    {
        throw std::runtime_error("This functionality isn't available. Compile NGen with NGEN_WITH_NETCDF=ON to enable NetCDF support");
    }
}

#endif // NGEN_WITH_NETCDF
