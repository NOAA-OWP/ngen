#include <NGenConfig.h>

#if NGEN_WITH_NETCDF
#include "NetCDFPerFeatureDataProvider.hpp"
#include "utilities/multithreading.hpp"
#include <mediator/UnitsHelper.hpp>

#include <netcdf>

#include <chrono>
#include <cmath>
#include <limits>
#include <regex>
#include <span>
#include <sstream>
#include <variant>

static const std::string TIME_VAR_NAME = "Time";
static const std::string TIME_DIM_NAME = "time";

// Where the time dimension is among the dimensions of a variable stored
// over the catchment and time dimensions, in either order, as forcing
// variables are; nothing for any other variable
static std::optional<std::size_t> forcing_time_axis(const netCDF::NcVar& var, const std::string& catchment_dim)
{
    if (var.getDimCount() != 2) {
        return std::nullopt;
    }
    const std::string dim0 = var.getDim(0).getName();
    const std::string dim1 = var.getDim(1).getName();
    if (dim0 == catchment_dim && dim1 == TIME_DIM_NAME) {
        return 1;
    }
    if (dim0 == TIME_DIM_NAME && dim1 == catchment_dim) {
        return 0;
    }
    return std::nullopt;
}

std::mutex data_access::NetCDFPerFeatureDataProvider::shared_providers_mutex;
std::map<std::string, std::shared_ptr<data_access::NetCDFPerFeatureDataProvider>> data_access::NetCDFPerFeatureDataProvider::shared_providers;

namespace data_access {

namespace {
    // The NetCDF library's default fill value for each numeric type, by
    // type id. Values and attributes of these types read as doubles
    const std::map<nc_type, double> DEFAULT_FILL_VALUES = {
        {NC_BYTE, NC_FILL_BYTE},
        {NC_UBYTE, NC_FILL_UBYTE},
        {NC_SHORT, NC_FILL_SHORT},
        {NC_USHORT, NC_FILL_USHORT},
        {NC_INT, NC_FILL_INT},
        {NC_UINT, NC_FILL_UINT},
        {NC_INT64, NC_FILL_INT64},
        {NC_UINT64, NC_FILL_UINT64},
        {NC_FLOAT, NC_FILL_FLOAT},
        {NC_DOUBLE, NC_FILL_DOUBLE},
    };

    // The values of a numeric attribute, none if the variable has no
    // numeric attribute by that name
    std::vector<double> get_numeric_att(const netCDF::NcVar& var, const std::string& name)
    {
        auto atts = var.getAtts();
        auto found = atts.find(name);
        if (found == atts.end() || !DEFAULT_FILL_VALUES.contains(found->second.getType().getId())) {
            return {};
        }
        std::vector<double> values(found->second.getAttLength());
        found->second.getValues(values.data());
        return values;
    }

    // The value of a numeric attribute holding a single value, if the
    // variable has one by that name
    std::optional<double> get_scalar_att(const netCDF::NcVar& var, const std::string& name)
    {
        auto values = get_numeric_att(var, name);
        if (values.size() != 1) {
            return std::nullopt;
        }
        return values.front();
    }

    // The stored values marking missing data: the _FillValue (the type's
    // default if no _FillValue attribute is set but fill mode is on,
    // except for byte types) and any missing_value attribute values. This
    // follows the masking of the netCDF4 Python module, apart from
    // valid_min, valid_max and valid_range, which aren't applied
    std::vector<double> get_missing_markers(const netCDF::NcVar& var)
    {
        std::vector<double> markers = get_numeric_att(var, "missing_value");

        std::optional<double> fill = get_scalar_att(var, "_FillValue");
        if (!fill) {
            bool fill_mode = false;
            var.getFillModeParameters(fill_mode);
            const nc_type type = var.getType().getId();
            auto default_fill = DEFAULT_FILL_VALUES.find(type);
            if (fill_mode && type != NC_BYTE && type != NC_UBYTE && default_fill != DEFAULT_FILL_VALUES.end()) {
                fill = default_fill->second;
            }
        }
        if (fill) {
            markers.push_back(*fill);
        }

        std::erase_if(markers, [](double marker) { return std::isnan(marker); });
        return markers;
    }
}

struct NetCDFPerFeatureDataProvider::VariableInfo
{
    // The variable's name in the file, which keys value_caches and the
    // thread caches. Storing it saves calling NcVar::getName(), which
    // would need the NetCDF library lock
    std::string name_;
    netCDF::NcVar ncvar_;

    // The variable's units: its units attribute, or for a well-known
    // field without one, the field's usual units
    std::string units_;

    // Packed values are unpacked as stored * scale_factor + add_offset
    double scale_factor_ = 1.0;
    double add_offset_ = 0.0;

    // Stored values marking missing data, which decode to NaN
    std::vector<double> missing_markers_;

    // Where the time dimension is among the variable's dimensions, if it
    // holds forcing values; see forcing_time_axis()
    std::optional<std::size_t> time_axis_;

    // Reads how to decode the variable's values from its attributes;
    // the caller must hold the NetCDF library lock
    VariableInfo(std::string name, netCDF::NcVar var, const std::string& catchment_dim, std::string units = "")
        : name_(std::move(name))
        , ncvar_(var)
        , units_(std::move(units))
        , scale_factor_(get_scalar_att(var, "scale_factor").value_or(1.0))
        , add_offset_(get_scalar_att(var, "add_offset").value_or(0.0))
        , missing_markers_(get_missing_markers(var))
        , time_axis_(forcing_time_axis(var, catchment_dim))
    {}

    // Whether the variable holds forcing values: whether it's stored
    // over the catchment and time dimensions, in either order
    bool holds_forcings() const
    {
        return time_axis_.has_value();
    }

    // Whether it's stored as (time, catchment-id) rather than
    // (catchment-id, time)
    bool time_major() const
    {
        return time_axis_ == 0;
    }

    // Turns stored values into the values they represent, with NaN for
    // missing data
    void decode(std::span<double> values) const
    {
        if (!missing_markers_.empty()) {
            for (double& v : values) {
                if (std::ranges::find(missing_markers_, v) != missing_markers_.end()) {
                    v = std::numeric_limits<double>::quiet_NaN();
                }
            }
        }
        unpack(values);
    }

    // Applies scale_factor and add_offset, leaving missing markers alone
    void unpack(std::span<double> values) const
    {
        if (scale_factor_ != 1.0 || add_offset_ != 0.0) {
            for (double& v : values) {
                v = v * scale_factor_ + add_offset_;
            }
        }
    }
};

std::shared_ptr<NetCDFPerFeatureDataProvider> NetCDFPerFeatureDataProvider::get_shared_provider(std::string input_path, time_t sim_start, time_t sim_end, utils::StreamHandler log_s)
{
    const std::lock_guard<std::mutex> lock(shared_providers_mutex);
    std::shared_ptr<NetCDFPerFeatureDataProvider> p;
    if(shared_providers.count(input_path) > 0){
        p = shared_providers[input_path];
    } else {
        p = std::make_shared<data_access::NetCDFPerFeatureDataProvider>(input_path, sim_start, sim_end, log_s);
        shared_providers[input_path] = p;
    }
    return p;
}

void NetCDFPerFeatureDataProvider::cleanup_shared_providers()
{
    const std::lock_guard<std::mutex> lock(shared_providers_mutex);
    // First lets try just emptying the map... if all goes well, everything will destruct properly on its own...
    shared_providers.clear();
}

std::optional<NetCDFPerFeatureDataProvider::TimeInfo>
NetCDFPerFeatureDataProvider::interpret_time_units(const std::string& units_str)
{
    TimeInfo info;
    info.epoch_start_time = std::nullopt;

    // set time unit and scale factor; the UDUNITS names and symbols of
    // each unit, except that "m" means minutes
    static const std::map<std::string, std::pair<TimeUnit, double>> units = {
        {"d",            {TimeUnit::Days,         86400}},
        {"day",          {TimeUnit::Days,         86400}},
        {"days",         {TimeUnit::Days,         86400}},

        {"h",            {TimeUnit::Hours,        3600}},
        {"hr",           {TimeUnit::Hours,        3600}},
        {"hour",         {TimeUnit::Hours,        3600}},
        {"hours",        {TimeUnit::Hours,        3600}},

        {"m",            {TimeUnit::Minutes,      60}},
        {"min",          {TimeUnit::Minutes,      60}},
        {"minute",       {TimeUnit::Minutes,      60}},
        {"minutes",      {TimeUnit::Minutes,      60}},

        {"s",            {TimeUnit::Seconds,      1}},
        {"sec",          {TimeUnit::Seconds,      1}},
        {"second",       {TimeUnit::Seconds,      1}},
        {"seconds",      {TimeUnit::Seconds,      1}},

        {"ms",           {TimeUnit::Milliseconds, 1.0e-3}},
        {"millisecond",  {TimeUnit::Milliseconds, 1.0e-3}},
        {"milliseconds", {TimeUnit::Milliseconds, 1.0e-3}},

        {"us",           {TimeUnit::Microseconds, 1.0e-6}},
        {"microsecond",  {TimeUnit::Microseconds, 1.0e-6}},
        {"microseconds", {TimeUnit::Microseconds, 1.0e-6}},

        {"ns",           {TimeUnit::Nanoseconds,  1.0e-9}},
        {"nanosecond",   {TimeUnit::Nanoseconds,  1.0e-9}},
        {"nanoseconds",  {TimeUnit::Nanoseconds,  1.0e-9}},
    };
    auto found = units.find(units_str);
    if (found == units.end())
    {
        return std::nullopt;
    }
    info.unit = found->second.first;
    info.scale_factor = found->second.second;

    return info;
}

std::time_t NetCDFPerFeatureDataProvider::parse_epoch(const std::string& date_str)
{
    // epoch_start attributes write dates as MM/DD/YYYY or MM/DD/YY;
    // rewrite those as YYYY-MM-DD, reading two-digit years as strptime does
    static const std::regex slashed_date(R"(^\s*(\d{1,2})/(\d{1,2})/(\d{4}|\d{2})(.*)$)");
    std::string text = date_str;
    std::smatch slashed;
    if (std::regex_match(date_str, slashed, slashed_date)) {
        std::string year = slashed[3].str();
        if (year.size() == 2) {
            const int two_digit_year = std::stoi(year);
            year = std::to_string(two_digit_year + (two_digit_year < 69 ? 2000 : 1900));
        }
        text = year + "-" + slashed[1].str() + "-" + slashed[2].str() + slashed[4].str();
    }

    static const std::regex pattern(
        R"(^\s*(\d{4})-(\d{1,2})-(\d{1,2}))"                      // date
        R"((?:(?:T|\s+)(\d{1,2}):(\d{2})(?::(\d{2})(?:\.(\d+))?)?)?)" // time of day
        R"(\s*(Z|UTC|([+-])(\d{2})(?::?(\d{2}))?)?\s*$)");            // UTC or an offset from it
    std::smatch m;
    if (!std::regex_match(text, m, pattern)) {
        throw std::runtime_error("Could not parse reference date '" + date_str + "'");
    }
    auto field = [&](std::size_t i) { return m[i].matched ? std::stoi(m[i].str()) : 0; };

    using namespace std::chrono;
    const year_month_day date = year{field(1)} / field(2) / field(3);
    const hours hour{field(4)};
    const minutes minute{field(5)};
    const seconds second{field(6)};
    // ok() rejects dates that don't exist, such as February 30
    if (!date.ok() || hour.count() > 23 || minute.count() > 59 || second.count() > 60) {
        throw std::runtime_error("Reference date '" + date_str + "' has a field out of range");
    }
    auto& fraction_digits = m[7];
    if (fraction_digits.matched && fraction_digits.str().find_first_not_of('0') != std::string::npos) {
        throw std::runtime_error("Reference date '" + date_str + "' has fractional seconds");
    }
    const seconds time_of_day = hour + minute + second;
    sys_seconds t = sys_days{date} + time_of_day;
    auto& offset_sign = m[9];
    if (offset_sign.matched) {
        const hours offset_hours{field(10)};
        const minutes offset_minutes{field(11)};
        if (offset_hours.count() > 23 || offset_minutes.count() > 59) {
            throw std::runtime_error("Reference date '" + date_str + "' has an offset from UTC out of range");
        }
        const minutes utc_offset = offset_hours + offset_minutes;
        t -= offset_sign.str() == "-" ? -utc_offset : utc_offset;
    }
    return t.time_since_epoch().count();
}

NetCDFPerFeatureDataProvider::TimeInfo NetCDFPerFeatureDataProvider::get_time_metadata(const VariableInfo& time_var)
{
    // read the meta data to get the time_unit
    // if absent, assume seconds
    TimeInfo info{TimeUnit::Seconds, 1, std::nullopt};
    // the reference date, from CF units of the form "<unit> since <date>"
    // or else the epoch_start attribute
    std::string epoch_str;

    // the units attribute, as read for the variable's VariableInfo; empty
    // if it's absent or couldn't be read
    const std::string& units_att_str = time_var.units_;

    // CF conventions may have units of the form "<unit> since <date>"
    std::string time_unit_str = units_att_str;
    const std::string since = " since ";
    const size_t since_pos = units_att_str.find(since);
    if ( since_pos != std::string::npos )
    {
        time_unit_str = units_att_str.substr(0, since_pos);
        epoch_str = units_att_str.substr(since_pos + since.length());
    }

    // set time unit and scale factor
    if ( auto parsed = interpret_time_units(time_unit_str) )
    {
        info = *parsed;
    }
    else if ( !time_unit_str.empty() )
    {
        throw std::runtime_error("In NetCDF file '" + file_path + "', the " + TIME_VAR_NAME
                                 + " variable's units '" + units_att_str + "' aren't a recognized time unit");
    }
    else {
        log_stream << "Warning using default time units\n";
    }
    assert(info.scale_factor != 0); // This should not happen.

    // fall back to the epoch_start attribute only when the units string didn't supply an epoch
    if ( epoch_str.empty() )
    {
        try {
            auto epoch_att = time_var.ncvar_.getAtt("epoch_start");

            if ( epoch_att.isNull() )
            {
                log_stream << "Warning using default epoch string\n";
            }
            else
            {
                epoch_att.getValues(epoch_str);
            }
        }
        catch(const netCDF::exceptions::NcException& e) {
            std::cerr<<e.what()<<std::endl;
            log_stream << "Warning using default epoch string\n";
        }
    }

    if ( !epoch_str.empty() )
    {
        info.epoch_start_time = parse_epoch(epoch_str);
    }

    return info;
}

NetCDFPerFeatureDataProvider::NetCDFPerFeatureDataProvider(std::string input_path, time_t sim_start, time_t sim_end, utils::StreamHandler log_s)
    : log_stream(log_s)
    , file_path(input_path)
    , sim_start_date_time_epoch(sim_start)
    , sim_end_date_time_epoch(sim_end)
{
    // Held for the whole constructor, including the private helpers it
    // calls that read from the file
    std::unique_lock netcdf_lock(multithreading::library_mutex<"netcdf">());

    //size_t sizep = 1073741824, nelemsp = 202481;
    //float preemptionp = 0.75;
    //nc_set_chunk_cache(sizep, nelemsp, preemptionp);

    //open the file
    try{
        nc_file = std::shared_ptr<netCDF::NcFile>(new netCDF::NcFile(input_path, netCDF::NcFile::read), [](netCDF::NcFile* file) {
            // Deleting an NcFile closes it if it's still open, which calls
            // into the NetCDF library
            std::unique_lock netcdf_lock(multithreading::library_mutex<"netcdf">());
            delete file;
        });
    }
    catch(const netCDF::exceptions::NcException& e){
        std::cerr<<"Error opening NetCDF file: "<<input_path<<std::endl;
        std::cerr<<e.what()<<std::endl;
        throw;
    }
    // check for the variables and dimension every forcing file has,
    // before anything uses them
    {
        auto require = [&](bool ok, const std::string& problem) {
            if (!ok) {
                throw std::runtime_error("NetCDF forcing file '" + input_path + "' " + problem);
            }
        };
        auto ids_var = nc_file->getVar("ids");
        require(!ids_var.isNull(), "has no \"ids\" variable");
        require(ids_var.getDimCount() == 1, "has an \"ids\" variable with "
                + std::to_string(ids_var.getDimCount()) + " dimensions, rather than 1");
        require(ids_var.getType().getTypeClass() == NC_STRING, "has an \"ids\" variable of type "
                + ids_var.getType().getName() + ", rather than string");
        catchment_dim_name = ids_var.getDim(0).getName();
        require(!nc_file->getDim(TIME_DIM_NAME).isNull(), "has no \"" + TIME_DIM_NAME + "\" dimension");
        require(nc_file->getDim(TIME_DIM_NAME).getSize() > 0, "has no timesteps");
        require(!nc_file->getVar(TIME_VAR_NAME).isNull(), "has no \"" + TIME_VAR_NAME + "\" variable");
    }

    // do a quick test of the netcdf variables to ensure
    // they are readable, especially if compressed with HDF5 filters
    // This also has the added benefit of warming up the chunk cache
    test_data_is_readable();

    //nc_get_chunk_cache(&sizep, &nelemsp, &preemptionp);
    //std::cout << "Chunk cache parameters: "<<sizep<<", "<<nelemsp<<", "<<preemptionp<<std::endl;
    read_variable_metadata();
    // uses the variables' VariableInfo
    align_cache_with_chunks();
    read_ids();
    read_times();
}

void NetCDFPerFeatureDataProvider::read_variable_metadata()
{
    //get the listing of all variables
    auto var_set = nc_file->getVars();

    // populate the ncvar and units caches...
    std::for_each(var_set.begin(), var_set.end(), [&](const auto& element)
    {
        std::string var_name = element.first;
        auto ncvar = nc_file->getVar(var_name);
        variable_names.push_back(var_name);
        value_caches.try_emplace(var_name);

        std::string native_units;
        try
        {
            auto units_att = ncvar.getAtt("units");
            if ( units_att.isNull() )
            {
                native_units = "";
            }
            else
            {
                units_att.getValues(native_units);
            }
        }
        catch(...)
        {
            native_units = "";
        }

        auto wkf = data_access::WellKnownFields.find(var_name);
        if (wkf != data_access::WellKnownFields.end() && native_units.empty()) {
            native_units = std::get<1>(wkf->second); // the field's usual units
        }

        VariableInfo info(var_name, ncvar, catchment_dim_name, native_units);
        ncvar_cache.emplace(var_name, info);

        if(wkf != data_access::WellKnownFields.end()){
            std::string can_name = std::get<0>(wkf->second); // the CSDMS name
            auto [existing, inserted] = ncvar_cache.emplace(can_name, info);
            if (inserted) {
                variable_names.push_back(can_name);
            } else {
                // another variable already has this CSDMS name
                auto& candidates = ambiguous_names[can_name];
                if (candidates.empty()) {
                    candidates.push_back(existing->second.name_);
                }
                candidates.push_back(var_name);
            }
        }
    });
}

void NetCDFPerFeatureDataProvider::read_ids()
{
    // read the variable ids, a 1-D string variable, as checked above
    auto ids = nc_file->getVar("ids");
    auto id_dim = ids.getDim(0);

    auto num_ids = id_dim.getSize();
    if (num_ids <= 0){
        throw std::runtime_error("NetCDF forcing file '" + file_path + "' has no features");
    }

    // include all catchments in the "default" chunk
    auto pair = std::pair<size_t, size_t>(0, num_ids);
    chunks.push_back(pair);

    //TODO: split into smaller slices if num_ids is large.
    cache_slice_c_size = num_ids;

    // allocate an array of character pointers 
    std::vector< char* > string_buffers(num_ids);

    // read the id strings
    ids.getVar(string_buffers.data());

    // copy the strings allocated by the C library, which may be NULL for
    // values never written, and free them
    std::vector<std::string> file_ids;
    for (const char* str : string_buffers) {
        file_ids.emplace_back(str ? str : "");
    }
    nc_free_string(num_ids, string_buffers.data());

    // initialize the map of catchment-name to offset location
    for (std::size_t loc = 0; loc < file_ids.size(); ++loc) {
        const std::string& id = file_ids[loc];
        if (id.empty()) {
            throw std::runtime_error("NetCDF forcing file '" + file_path + "' has an empty catchment id at position "
                                     + std::to_string(loc) + " of \"ids\"");
        }
        if (!id_pos.emplace(id, loc).second) {
            throw std::runtime_error("NetCDF forcing file '" + file_path + "' has catchment id '" + id
                                     + "' more than once in \"ids\"");
        }
        loc_ids.push_back(id);
    }
}

void NetCDFPerFeatureDataProvider::read_times()
{
    // now get the size of the time dimension
    auto num_times = nc_file->getDim(TIME_DIM_NAME).getSize();

    // allocate storage for the raw time array
    std::vector<double> raw_time(num_times);

    // get the time variable, whose VariableInfo the loop over the
    // variables above made
    const VariableInfo& time_variable = ncvar_cache.at(TIME_VAR_NAME);
    const netCDF::NcVar& time_var = time_variable.ncvar_;

    // Time is stored over just the time dimension, as the forcings engine
    // writes it, or over both dimensions with every catchment's times the
    // same; read the first catchment's
    std::vector<size_t> start, count;
    std::vector<std::string> time_dims;
    for (const auto& dim : time_var.getDims()) {
        time_dims.push_back(dim.getName());
    }
    if (time_dims.size() == 1 && time_dims[0] == TIME_DIM_NAME) {
        start = {0};
        count = {num_times};
    } else if (time_variable.time_axis_) {
        start = {0, 0};
        count = {1, 1};
        count[*time_variable.time_axis_] = num_times;
    } else {
        std::string dims_str;
        for (const auto& d : time_dims) {
            dims_str += (dims_str.empty() ? "" : ", ") + d;
        }
        throw std::runtime_error("In NetCDF file '" + file_path + "', the " + TIME_VAR_NAME
                                 + " variable has dimensions (" + dims_str + "), rather than ("
                                 + TIME_DIM_NAME + ") or (" + TIME_DIM_NAME + " and " + catchment_dim_name + ")");
    }
    time_var.getVar(start, count, &raw_time[0]);
    time_variable.unpack(raw_time);

    // read the time metadata (unit, scale factor and reference epoch)
    TimeInfo time_info = get_time_metadata(time_variable);
    time_unit = time_info.unit;

    // scale the time to account for time units and epoch_start
    // TODO make sure this happens with a FMA instruction
    time_vals.resize(raw_time.size());
    std::transform(raw_time.begin(), raw_time.end(), time_vals.begin(),
        [&](const auto& n){return n * time_info.scale_factor + time_info.epoch_start_time.value_or(0); });

    start_time = time_vals.at(0);

    // determine the stride of the time array; a single timestep has no
    // interval to take it from, so take it to be an hour
    time_stride = time_vals.size() > 1 ? time_vals[1] - time_vals[0] : 3600;
    if (!std::isfinite(time_stride) || time_stride <= 0) {
        throw std::runtime_error("In NetCDF file '" + file_path + "', " + TIME_VAR_NAME
                                 + " values must increase, but the first time step is "
                                 + std::to_string(time_stride) + " seconds");
    }

    #ifndef NCEP_OPERATIONS
    // verify the time stride
    for (size_t i = 1; i < time_vals.size(); ++i) {
        double interval = time_vals[i] - time_vals[i-1];
        if (!std::isfinite(interval) || std::abs(interval - time_stride) > 1e-6) {
            log_stream << "Error: Time intervals are not constant in forcing file\n";
            throw std::runtime_error("Time intervals in forcing file '" + file_path + "' are not constant: "
                                     + std::to_string(interval) + " seconds at step " + std::to_string(i)
                                     + ", rather than " + std::to_string(time_stride));
        }
    }
    #endif

    stop_time = time_vals.back() + time_stride;

    sim_to_data_time_offset = sim_start_date_time_epoch - start_time;
}

void NetCDFPerFeatureDataProvider::hint_shared_provider_id(const std::string& id)
{
    std::lock_guard l{hinted_ids_mutex};
    hinted_ids.emplace(id);
}

void NetCDFPerFeatureDataProvider::maybe_update_chunks_with_hints()
{
    if (hinted_ids_done.test()) {
        return;
    }

    std::lock_guard l{hinted_ids_mutex};

    if (hinted_ids.size() == 0){
        hinted_ids_done.test_and_set();
        return;
    }

    // Base cases covered in other ctor
    if (hinted_ids.size() == get_ids().size()) {
        assert(chunks.size() == 1);
        hinted_ids.clear();
        hinted_ids_done.test_and_set();
        return;
    }
    // Map from nc cat-id index to cat-id; sorted by index position
    std::map<std::size_t, std::string> idx_map;

    // find the "id_pos" keys that are in "hinted_ids"
    for (auto const& [id, pos] : id_pos) {
        if (hinted_ids.count(id) > 0) {
            // Here we know the id AND its position in the index
            idx_map.emplace(pos, id);
        }
    }
    // throw before modifying any state, so a failed call leaves it unchanged
    if(idx_map.empty()){
        throw std::runtime_error("NetCDF source has no ids matching the domain hinted ids.");
    }

    // get rid of "default" chunks, we will build them here
    chunks.clear();
    // remove "id_pos" keys that are not in "hinted_ids"; the rest are re-added below
    id_pos.clear();
    // NOTE: not sure if there are dependencies elsewhere on the ordering of this vector.
    // refill in the expected order just to be on the safe side.
    loc_ids.clear();
    // Build chunks where a chunk has:
    // a starting nc index
    // the length of the chunk relative to its starting index
    //
    // While building the chunks, rebase nc indices to now "internal" cache indices
    for (auto const& [nc_idx, id] : idx_map) {
        // Start a new chunk at the first id, or after a gap in nc indices
        if (chunks.empty() || nc_idx != chunks.back().first + chunks.back().second) {
            chunks.emplace_back(nc_idx, 1);
        } else {
            ++chunks.back().second;
        }

        // NOTE: update "id_pos" with new "internal" index
        id_pos[id] = loc_ids.size();

        // push id back onto "loc_ids" in its original order
        loc_ids.push_back(id);
    }

    // minor gains
    loc_ids.shrink_to_fit();

    cache_slice_c_size = loc_ids.size();

    // TODO: improve this; we only want this method to "do something" once.
    //       invariant is, weakly, enforced by prelude guard clause.
    hinted_ids.clear();

    hinted_ids_done.test_and_set();
}

NetCDFPerFeatureDataProvider::~NetCDFPerFeatureDataProvider() = default;

void NetCDFPerFeatureDataProvider::finalize()
{
    if (nc_file != nullptr) {
        std::unique_lock netcdf_lock(multithreading::library_mutex<"netcdf">());
        nc_file->close();
    }
    // Outside the lock, since nc_file's deleter takes it
    nc_file = nullptr;
}

std::span<const std::string> NetCDFPerFeatureDataProvider::get_available_variable_names() const
{
    return variable_names;
}

const std::vector<std::string>& NetCDFPerFeatureDataProvider::get_ids() const
{
    return loc_ids;
}

/** Return the first valid time for which data from the request variable  can be requested */
long NetCDFPerFeatureDataProvider::get_data_start_time() const
{
    //return start_time;
    //FIXME: Matching behavior from CsvPerFeatureForcingProvider, but both are probably wrong!
    return sim_start_date_time_epoch; // return start_time + sim_to_data_time_offset;
}

/** Return the last valid time for which data from the requested variable can be requested */
long NetCDFPerFeatureDataProvider::get_data_stop_time() const
{
    //return stop_time;
    //FIXME: Matching behavior from CsvPerFeatureForcingProvider, but both are probably wrong!
    return sim_end_date_time_epoch; // return end_time + sim_to_data_time_offset;
}

long NetCDFPerFeatureDataProvider::record_duration() const
{
    return time_stride;
}

size_t NetCDFPerFeatureDataProvider::get_ts_index_for_time(const time_t &epoch_time) const
{
    if (start_time <= epoch_time && epoch_time < stop_time)
    {
        double offset = epoch_time - start_time;
        offset /= time_stride;
        return size_t(offset);
    }
    else
    {
        std::stringstream ss;
        ss << "The time " << (long long)epoch_time << " is outside the range [" << (long long)start_time << ", "
           << (long long)stop_time << ") of NetCDF forcing file '" << file_path << "'\n" << SOURCE_LOC;
        throw std::out_of_range(ss.str());
    }
}

namespace {
namespace cache {
    std::size_t page_count(std::size_t c_size, std::size_t line_size) {
        return (c_size / line_size) + std::min<std::size_t>(1, c_size % line_size);
    }

    std::size_t page_p_idx(std::size_t c_idx, std::size_t line_size) {
        return c_idx / line_size;
    }

    std::size_t page_c_idx(std::size_t c_idx, std::size_t line_size) {
        std::size_t p_idx = page_p_idx(c_idx, line_size);
        return p_idx * line_size;
    }

    std::size_t page_p_idx_to_c_idx(std::size_t p_idx, std::size_t line_size) {
        return p_idx * line_size;
    }

    std::size_t page_entry_j_idx(std::size_t c_idx, std::size_t line_size) {
        return c_idx - page_c_idx(c_idx, line_size);
    }

    std::size_t page_cache_line_size(std::size_t c_idx, std::size_t c_size, std::size_t line_size) {
        std::size_t ps = page_count(c_size, line_size);
        std::size_t p_idx = page_p_idx(c_idx, line_size);
        assert(p_idx < ps);
        if (p_idx < ps - 1) {
            return line_size;
        }
        std::size_t ts = page_c_idx(c_idx, line_size);
        return c_size - ts;
    }

    std::size_t page_entry_idx(std::size_t i_idx, std::size_t c_idx, std::size_t c_size, std::size_t line_size) {
        std::size_t pls = page_cache_line_size(c_idx, c_size, line_size);
        // NOTE: this is relative to the default line size
        std::size_t pj_idx = page_entry_j_idx(c_idx, line_size);
        return (pls * i_idx) + pj_idx;
    }
}
}

auto NetCDFPerFeatureDataProvider::load_page(VariableInfo const& var, std::size_t page_c_idx) -> page_buffer_type
{
  std::size_t cache_line_size = cache_slice_t_size;
  std::size_t page_cache_line_size = cache::page_cache_line_size(page_c_idx, time_vals.size(), cache_line_size);
  // Allocate before taking the NetCDF library lock, so that it is held
  // only for the reads
  auto cached = std::make_shared<std::vector<double>>(get_ids().size() * page_cache_line_size);

  std::vector<std::size_t> start(2), count(2);
  // A time-major variable's chunks are read here, then transposed into
  // the page's catchment-major layout
  std::vector<double> transposed;

  std::unique_lock netcdf_lock(multithreading::library_mutex<"netcdf">());

  // read each chunk and add it to "cached"
  std::size_t idx = 0;
  for(auto const& chunk: chunks){
    // chunk start index = chunk.first;
    // chunk length      = chunk.second;

    // NOTE: in the first iteration, we might read more data in the Time
    // dimension than we 'need'. b.c. we read from:
    // 'c_idx1 - (c_idx1 % cache_slice_t_size)' to the end of the cache line.
    // so, if 'c_idx1 % cache_slice_t_size > 0' we will read
    // 'c_idx1 % cache_slice_t_size * next_chunk_idx' more values than we 'need' to.
    if (var.time_major()) {
      start = {page_c_idx, chunk.first};
      count = {page_cache_line_size, chunk.second};
      transposed.resize(chunk.second * page_cache_line_size);
      var.ncvar_.getVar(start, count, transposed.data());
      for (std::size_t t = 0; t < page_cache_line_size; ++t) {
        for (std::size_t c = 0; c < chunk.second; ++c) {
          (*cached)[idx + c * page_cache_line_size + t] = transposed[t * chunk.second + c];
        }
      }
    } else {
      start = {chunk.first, page_c_idx};
      count = {chunk.second, page_cache_line_size};
      var.ncvar_.getVar(start,count,&(*cached)[idx]);
    }
    idx += chunk.second * page_cache_line_size;
  }
  netcdf_lock.unlock();

  var.decode(*cached);
  return cached;
}

double NetCDFPerFeatureDataProvider::get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m) 
{
    /*
     * this cache is made up of pages.
     * each page contains cache lines.
     * a page is a flattened 2d array with dimensions catchment-id, time (i, j).
     * connecting pages contiguously along cache lines (left-to-right), forms the time series.
     * this implicit dimension is named the 'c' dimension.
     * the 'c' dimension is the (page idx + page relative cache line j idx) * cache line size.
     * 'c' is a forcing time step idx.
     *
     * each line, dim i, contains forcing data for a cache line sized time period for a single catchment.
     * there are n cache lines in a page.
     * n is the number of catchments a single ngen process is responsible for.
     * pages are divided by _cache line size_ (index p).
     * cache line size equates to n forcing time steps.
     * a page only contains data for a single forcing variable.
     *
     * if the forcing time dim size is not divisible by the cache line size,
     * the last cache page will have a cache line length = forcing time dim size % cache line size.
     *
     * dimensions and index naming conventions:
     * i dim: cache line index (rows)
     * j dim: cache line columns relative to page
     * c dim: columns across all cache lines (not relative to page)
     * p dim: page index
     */

    if (selector.get_duration_secs() <= 0) {
        throw std::invalid_argument("Read of '" + selector.get_variable_name() + "' for '" + selector.get_id()
                                    + "' from NetCDF file '" + file_path + "' has a duration of "
                                    + std::to_string(selector.get_duration_secs())
                                    + " seconds; a read must cover some time");
    }

    // update chunks during the first timestep, and no-op thereafter;
    // 'maybe_update_chunks_with_hints' clears 'hinted_ids'
    // assumes all id's will have been hinted before 'get_value' is called.
    maybe_update_chunks_with_hints();

    auto const& var = get_ncvar(selector.get_variable_name());
    if (!var.holds_forcings()) {
        throw std::runtime_error("Variable '" + selector.get_variable_name() + "' in NetCDF file '" + file_path
                                 + "' isn't stored over the " + TIME_DIM_NAME + " and " + catchment_dim_name
                                 + " dimensions, so holds no forcing values");
    }
    auto const& variable_name = var.name_;
    auto& variable_cache = value_caches.at(variable_name);

    auto const found_id = id_pos.find(selector.get_id());
    if (found_id == id_pos.end()) {
        throw std::out_of_range("Catchment '" + selector.get_id() + "' isn't among those read from NetCDF forcing file '"
                                + file_path + "'");
    }
    auto const i_idx = found_id->second;

    auto const init_time = selector.get_init_time();
    auto const end_time = init_time + selector.get_duration_secs();
    
    const size_t c_idx1 = get_ts_index_for_time(init_time);
    size_t c_idx2;
    try {
        c_idx2 = get_ts_index_for_time(end_time-1); // Don't include next timestep when duration % timestep = 0
    }
    catch(const std::out_of_range &e){
        c_idx2 = get_ts_index_for_time(stop_time-1); //to the edge
    }

    const std::size_t read_len = c_idx2 - c_idx1 + 1;

    const std::size_t cache_line_size = cache_slice_t_size;
    const std::size_t p_idx = cache::page_p_idx(c_idx1, cache_line_size);
    // pages spanned by [c_idx1, c_idx2]; ceil(read_len / line_size) undercounts
    // when the range starts mid-page and crosses a page boundary
    const std::size_t n_page_accesses = cache::page_p_idx(c_idx2, cache_line_size) - p_idx + 1;

    // Only evict pages before the first one this read needs. Every
    // thread in a time step reads the same pages, so when a read spans
    // two pages, evicting below the second would drop the first while
    // this or another thread still needs it
    const std::size_t eviction_floor = cache::page_p_idx_to_c_idx(p_idx, cache_line_size);

    // Drop references to arrays of stale forcings values here,
    // before reading or waiting on reads, to avoid or limit
    // spikes in memory footprint as new forcings get read in
    auto& variable_thread_cache = thread_caches.get()[variable_name];
    multithreading::evict_stale_values(variable_thread_cache, eviction_floor);

    for( size_t i = 0; i < n_page_accesses; i++ ) {
        // rows: catchments; columns: time;
        // stride between rows is 'cache_line_size'
        const std::size_t ith_p_idx = p_idx + i;
        const std::size_t page_c_idx = cache::page_p_idx_to_c_idx(ith_p_idx, cache_line_size);

        const cache_key_type page_key = page_c_idx;

        if (variable_thread_cache.contains(page_key))
            continue;

        auto [slot_for_page_key, just_inserted] = variable_cache.find_or_insert(page_key, eviction_floor);

        page_buffer_type cached;
        if (just_inserted) {
            try {
                cached = load_page(var, page_c_idx);
            } catch (...) {
                // Mark the slot failed, so that readers waiting for this
                // page, and later ones, throw rather than wait forever,
                // and pass on this read's error
                slot_for_page_key.fail();
                throw;
            }
            slot_for_page_key.fill(cached, true);
        } else {
            auto got = slot_for_page_key.get();
            if (std::holds_alternative<multithreading::failed_fill>(got)) {
                throw std::runtime_error("An earlier read of variable '" + variable_name + "' from NetCDF file '"
                                         + file_path + "', for the timesteps from index "
                                         + std::to_string(page_c_idx) + ", failed");
            }
            cached = std::get<page_buffer_type>(std::move(got));
        }
        variable_thread_cache.emplace(page_key, cached);
    }

    std::size_t c_idx = c_idx1;

    std::vector<double> raw_values;
    raw_values.reserve(read_len);

    for( size_t i = 0; i < n_page_accesses; i++ ) {
        // rows: catchments; columns: time;
        // stride between rows is 'cache_line_size'
        const std::size_t ith_p_idx = p_idx + i;
        const std::size_t page_c_idx = cache::page_p_idx_to_c_idx(ith_p_idx, cache_line_size);
        const std::size_t page_cache_line_size = cache::page_cache_line_size(page_c_idx, time_vals.size(), cache_line_size);
        auto locally_cached = variable_thread_cache.at(page_c_idx).get();
        // Find all values in the current cache slice and push them onto raw_values
        while(c_idx >= page_c_idx &&
              c_idx < page_c_idx + page_cache_line_size &&
              c_idx <= c_idx2){
            std::size_t idx = cache::page_entry_idx(i_idx, c_idx, time_vals.size(), cache_line_size);
            double value = locally_cached->at(idx);
            raw_values.push_back(value);
            c_idx++;
        }
    }

    assert(raw_values.size() == read_len);
    double rvalue = 0.0;

    double t1 = time_vals[c_idx1];
    double t2 = time_vals[c_idx2];

    // The first and last timesteps may be only partly inside the window
    // [init_time, end_time); weight their values by the fraction that is
    double a , b = 0.0;
    if (raw_values.size() == 1) {
        a = (end_time - init_time) / time_stride;
    } else {
        a = (t1 + time_stride - init_time) / time_stride;
    }
    rvalue += (a * raw_values[0]);

    for( size_t i = 1; i < raw_values.size() -1; ++i )
    {
        rvalue += raw_values[i];
    }

    if (  raw_values.size() > 1) // likewise the last data value may not be fully in the window
    {
        b = (end_time - t2) / time_stride;
        rvalue += (b * raw_values.back() );
    }

    // account for the resampling methods
    switch(m)
    {
        case SUM:   // we already have the sum so do nothing
            ;
        break;

        case MEAN: 
        { 
            // This is getting a length weighted mean
            // the data values where already scaled for where there was only partial use of a data value
            // so we just need to do a final scale to account for the difference between time_stride and duration_s

            double scale_factor = (selector.get_duration_secs() > time_stride ) ? (time_stride / selector.get_duration_secs()) : (1.0 / (a + b));
            rvalue *= scale_factor;
        }
        break;

        default:
            ;
    }

    const std::string& native_units = var.units_;

    try 
    {
        return UnitsHelper::get_converted_value(native_units, rvalue, selector.get_output_units());
    }
    catch (UnitsHelper::unit_conversion_exception& uce)
    {
        uce.provider_model_name = "NetCDFPerFeatureDataProvider(" + file_path + ")";
        uce.provider_var_name = selector.get_variable_name();
        throw;
    }

    return rvalue;
}

std::vector<double> NetCDFPerFeatureDataProvider::get_values(const CatchmentAggrDataSelector& selector, data_access::ReSampleMethod m)
{
    return std::vector<double>(1, get_value(selector, m));
}

// private:

auto NetCDFPerFeatureDataProvider::get_ncvar(const std::string& name) const -> VariableInfo const& {
    auto ambiguous = ambiguous_names.find(name);
    if (ambiguous != ambiguous_names.end()) {
        std::string candidates;
        for (const auto& candidate : ambiguous->second) {
            candidates += (candidates.empty() ? "'" : ", '") + candidate + "'";
        }
        throw std::runtime_error("Variable '" + name + "' is ambiguous in NetCDF forcing file '" + file_path
                                 + "': it is the CSDMS name of each of " + candidates
                                 + "; read one of them by its name in the file");
    }

    auto cache_hit = ncvar_cache.find(name);
    if(cache_hit != ncvar_cache.end()){
        return cache_hit->second;
    }

    throw std::runtime_error("Got request for variable " + name + " but it was not found in the cache. This should not happen." + SOURCE_LOC);
}

void NetCDFPerFeatureDataProvider::test_data_is_readable() {
    // Try to read one element from each variable to test if NetCDF/HDF5 linking works for compressed data
    auto var_set = nc_file->getVars();
    for (const auto& var_pair : var_set) {
        const std::string& var_name = var_pair.first;
        auto var = var_pair.second;
        // Skip scalar variables
        if (var.getDimCount() == 0) continue;
        std::vector<size_t> start(var.getDimCount(), 0);
        std::vector<size_t> count(var.getDimCount(), 1);
        try {
            if (var.getType().getTypeClass() == NC_STRING) {
                // Handle string variable
                std::vector<char*> buffer(1);
                var.getVar(start, count, buffer.data());
                nc_free_string(1, buffer.data());
            } else if (DEFAULT_FILL_VALUES.contains(var.getType().getId())) {
                // Handle a variable of any numeric type, which reads as double
                double val;
                var.getVar(start, count, &val);
            } else {
                // Skip other types
                continue;
            }
        } catch (const netCDF::exceptions::NcException& e) {
            std::string err_str = e.what();
            // libnetcdf puts a lot of noise in the error message
            // so filter it out for clarity...
            size_t file_pos = err_str.find("file:");
            if (file_pos != std::string::npos) {
                err_str = err_str.substr(0, file_pos);
            }
            std::string msg = "Error reading " + var_name + " variable: " + err_str;
            // If ngen isn't directly linked against HDF5, then loading hdf5 plugin filters
            // fails, and compressed data cannot be read.  So check for filter issues
            // and provide a more helpful error message.
            if (err_str.find("NetCDF") != std::string::npos && (err_str.find("filter") != std::string::npos || err_str.find("Filter") != std::string::npos)) {
                msg +=  "This may happen if HDF5 isn't properly linked with ngen. "
                        "Ensure the build uses NGEN_WITH_HDF5. "
                        "Also check the $HDF5_PLUGIN_PATH environment variable and/or "
                        "nc-config --plugindir to ensure filter plugins are available.";
            }
            throw std::runtime_error(msg);
        }
    }
}

void NetCDFPerFeatureDataProvider::align_cache_with_chunks()
{
    auto num_times = nc_file->getDim(TIME_DIM_NAME).getSize();

    // in the file's order of variables
    auto var_set = nc_file->getVars();
    for (const auto& var_pair : var_set) {
        const std::string& var_name = var_pair.first;
        const VariableInfo& info = ncvar_cache.at(var_name);
        // Skip the Time variable itself, look for data variables over catchments and time
        if (var_name == TIME_VAR_NAME || !info.holds_forcings()) {
            continue;
        }
        netCDF::NcVar::ChunkMode mode;
        std::vector<size_t> chunk_sizes;
        info.ncvar_.getChunkingParameters(mode, chunk_sizes);
        if (mode == netCDF::NcVar::ChunkMode::nc_CHUNKED && chunk_sizes.size() == 2) {
            cache_slice_t_size = chunk_sizes[*info.time_axis_];
            break;  // Use the first suitable chunk size found
        }
    }
    if (num_times && cache_slice_t_size > num_times){
        cache_slice_t_size = num_times;
    }
    //At this point the time slice cache is either aligned with the chunking of the data variables
    //or set to the default value.
    return;
}

}

#endif
