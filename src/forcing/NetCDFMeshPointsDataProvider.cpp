#include <NGenConfig.h>

#if NGEN_WITH_NETCDF
#include "NetCDFMeshPointsDataProvider.hpp"
#include <UnitsHelper.hpp>

#include <netcdf>

#include <chrono>
#include <sstream>

namespace data_access {

// Out-of-line class definition after forward-declaration so that the
// header doesn't need #include <netcdf> for NcVar to be a complete
// type there
struct NetCDFMeshPointsDataProvider::metadata_cache_entry {
    netCDF::NcVar ncVar;
    std::string units;
    double scale_factor;
    double offset;
};

NetCDFMeshPointsDataProvider::NetCDFMeshPointsDataProvider(std::string input_path, time_point_type sim_start, time_point_type sim_end)
    : sim_start_date_time_epoch(sim_start)
    , sim_end_date_time_epoch(sim_end)
{
    nc_file = std::make_shared<netCDF::NcFile>(input_path, netCDF::NcFile::read);

    auto num_times = nc_file->getDim("time").getSize();
    auto time_var = nc_file->getVar("Time");

    if (time_var.getDimCount() != 1) {
        throw std::runtime_error("'Time' variable has dimension other than 1");
    }

    auto time_unit_att = time_var.getAtt("units");
    std::string time_unit_str;

    time_unit_att.getValues(time_unit_str);
    if (time_unit_str != "minutes since 1970-01-01 00:00:00 UTC") {
        throw std::runtime_error("Time units not exactly as expected");
    }

    std::vector<std::chrono::duration<double, std::ratio<60>>> raw_time(num_times);
    time_var.getVar(raw_time.data());

    time_vals.reserve(num_times);
    for (int i = 0; i < num_times; ++i) {
        // Assume that the system clock's epoch matches Unix time.
        // This is guaranteed from C++20 onwards
        time_vals.push_back(time_point_type(std::chrono::duration_cast<time_point_type::duration>(raw_time[i])));
    }

    time_stride = std::chrono::duration_cast<std::chrono::seconds>(time_vals[1] - time_vals[0]);

    // verify the time stride
    for( size_t i = 1; i < time_vals.size() -1; ++i)
    {
        auto tinterval = time_vals[i+1] - time_vals[i];

        if ( tinterval - time_stride > std::chrono::microseconds(1) ||
             time_stride - tinterval > std::chrono::microseconds(1) )
        {
            throw std::runtime_error("Time intervals in forcing file are not constant");
        }
    }
}

NetCDFMeshPointsDataProvider::~NetCDFMeshPointsDataProvider() = default;

void NetCDFMeshPointsDataProvider::finalize()
{
    ncvar_cache.clear();

    if (nc_file != nullptr) {
        nc_file->close();
    }
    nc_file = nullptr;
}

boost::span<const std::string> NetCDFMeshPointsDataProvider::get_available_variable_names() const
{
    return variable_names;
}

long NetCDFMeshPointsDataProvider::get_data_start_time() const
{
    return std::chrono::system_clock::to_time_t(time_vals[0]);
#if 0
    //return start_time;
    //FIXME: Matching behavior from CsvMeshPointsForcingProvider, but both are probably wrong!
    return sim_start_date_time_epoch.time_since_epoch().count(); // return start_time + sim_to_data_time_offset;
#endif
}

long NetCDFMeshPointsDataProvider::get_data_stop_time() const
{
    return std::chrono::system_clock::to_time_t(time_vals.back() + time_stride);
#if 0
    //return stop_time;
    //FIXME: Matching behavior from CsvMeshPointsForcingProvider, but both are probably wrong!
    return sim_end_date_time_epoch.time_since_epoch().count(); // return end_time + sim_to_data_time_offset;
#endif
}

long NetCDFMeshPointsDataProvider::record_duration() const
{
    return std::chrono::duration_cast<std::chrono::seconds>(time_stride).count();
}

size_t NetCDFMeshPointsDataProvider::get_ts_index_for_time(const time_t &epoch_time_in) const
{
    // time_t in simulation engine's time frame - i.e. seconds, starting at 0
    auto epoch_time = sim_start_date_time_epoch + std::chrono::seconds(epoch_time_in);

    auto start_time = time_vals.front();
    auto stop_time = time_vals.back() + time_stride;

    if (start_time <= epoch_time && epoch_time < stop_time)
    {
        auto offset = epoch_time - start_time;
        auto index = offset / time_stride;
        return index;
    }
    else
    {
        std::stringstream ss;
        ss << "The value " << std::chrono::system_clock::to_time_t(epoch_time) << " was not in the range ["
           << std::chrono::system_clock::to_time_t(start_time) << ", "
           << std::chrono::system_clock::to_time_t(stop_time) << ")\n"
           << SOURCE_LOC << "\n";
        ss << "Off by " << std::chrono::system_clock::to_time_t(epoch_time) - std::chrono::system_clock::to_time_t(start_time) << "\n";
        throw std::out_of_range(ss.str().c_str());
    }
}

void NetCDFMeshPointsDataProvider::get_values(const selection_type& selector, boost::span<data_type> data)
{
    if (!boost::get<AllPoints>(&selector.points)) throw std::runtime_error("Not implemented - only all_points");

    cache_variable(selector.variable_name);

    auto const& metadata = ncvar_cache[selector.variable_name];
    std::string const& source_units = metadata.units;

    size_t time_index = get_ts_index_for_time(std::chrono::system_clock::to_time_t(selector.init_time));

    metadata.ncVar.getVar({time_index, 0}, {1, data.size()}, data.data());

    for (auto& value : data) {
        value = value * metadata.scale_factor + metadata.offset;
    }

    // These mass and and volume flux density units are very close to
    // numerically identical for liquid water at atmospheric
    // conditions, and so we currently treat them as interchangeable
    bool RAINRATE_equivalence =
        selector.variable_name == "RAINRATE" &&
        source_units == "mm s^-1" &&
        selector.output_units == "kg m-2 s-1";

    if (!RAINRATE_equivalence) {
        UnitsHelper::convert_values(source_units, data.data(), selector.output_units, data.data(), data.size());
    }
}

NetCDFMeshPointsDataProvider::data_type NetCDFMeshPointsDataProvider::get_value(const selection_type& selector, ReSampleMethod m)
{
    throw std::runtime_error("Not implemented - access chunks of the mesh");

    return 0.0;
}

void NetCDFMeshPointsDataProvider::cache_variable(std::string const& var_name)
{
    if (ncvar_cache.find(var_name) != ncvar_cache.end()) return;

    auto ncvar = nc_file->getVar(var_name);
    variable_names.push_back(var_name);

    std::string native_units;
    auto units_att = ncvar.getAtt("units");
    units_att.getValues(native_units);

    double scale_factor = 1.0;
    try {
        auto scale_var = ncvar.getAtt("scale_factor");
        if (!scale_var.isNull()) {
            scale_var.getValues(&scale_factor);
        }
    } catch (...) {
        // Assume it's just not present, and so keeps the default value
    }

    double offset = 0.0;
    try {
        auto offset_var = ncvar.getAtt("add_offset");
        if (!offset_var.isNull()) {
            offset_var.getValues(&offset);
        }
    } catch (...) {
        // Assume it's just not present, and so keeps the default value
    }

    ncvar_cache[var_name] = {ncvar, native_units, scale_factor, offset};
}

}

#endif
