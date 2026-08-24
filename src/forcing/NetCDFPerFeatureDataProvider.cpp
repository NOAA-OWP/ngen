#include <NGenConfig.h>

#if NGEN_WITH_NETCDF
#include "NetCDFPerFeatureDataProvider.hpp"
#include <mediator/UnitsHelper.hpp>

#include <netcdf>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include "Logger.hpp"

using namespace std;
std::stringstream netcdf_ss;

std::mutex data_access::NetCDFPerFeatureDataProvider::shared_providers_mutex;
std::map<std::string, std::shared_ptr<data_access::NetCDFPerFeatureDataProvider>> data_access::NetCDFPerFeatureDataProvider::shared_providers;

// limit access outside of compilation unit.
namespace {
    const size_t N_EXPECTED_FORCING_VARS = 8;
}

namespace data_access {

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

    std::string time_unit_str = units_str;
    std::string unit_epoch_str = "";

    // CF conventions may have units of the form "<unit> since <date>"
    std::string since = " since ";
    size_t since_pos = units_str.find(since);
    if(since_pos != std::string::npos)
    {
        time_unit_str = units_str.substr(0, since_pos);
        unit_epoch_str = units_str.substr(since_pos + since.length());
    }

    if(!unit_epoch_str.empty())
    {
        //CF convention in time units formats as "YYYY-MM-DD HH:MM:SS"
        info.epoch_start_time = parse_epoch(unit_epoch_str, "%Y-%m-%d %H:%M:%S");
    }

    // set time unit and scale factor
    if ( time_unit_str == "h" || time_unit_str == "hours")
    {
        info.unit = TIME_HOURS;
        info.scale_factor = 3600;
    }
    else if ( time_unit_str == "m" || time_unit_str == "minutes" )
    {
        info.unit = TIME_MINUTES;
        info.scale_factor = 60;
    }
    else if ( time_unit_str ==  "s" || time_unit_str == "seconds" )
    {
        info.unit = TIME_SECONDS;
        info.scale_factor = 1;
    }
    else if ( time_unit_str ==  "ms" || time_unit_str == "milliseconds" )
    {
        info.unit = TIME_MILLISECONDS;
        info.scale_factor = 1.0e-3;
    }
    else if ( time_unit_str ==  "us" || time_unit_str == "microseconds" )
    {
        info.unit = TIME_MICROSECONDS;
        info.scale_factor = 1.0e-6;
    }
    else if ( time_unit_str ==  "ns" || time_unit_str == "nanoseconds" )
    {
        info.unit = TIME_NANOSECONDS;
        info.scale_factor = 1.0e-9;
    }
    else
    {
        return std::nullopt;
    }

    return info;
}

std::time_t NetCDFPerFeatureDataProvider::parse_epoch(const std::string& epoch_str, const std::string& format)
{
    std::tm tm{};
    std::stringstream s(epoch_str);
    s >> std::get_time(&tm, format.c_str());
    if (s.fail()) {
        throw std::runtime_error("Could not parse epoch '" + epoch_str + "' with format '" + format + "'");
    }
    // timegm is not available on Windows; see Simulation_Time.h
    return timegm(&tm);
}

NetCDFPerFeatureDataProvider::TimeInfo NetCDFPerFeatureDataProvider::get_time_metadata(const netCDF::NcVar& time_var)
{
    // read the meta data to get the time_unit
    // if absent, assume seconds
    TimeInfo info{TIME_SECONDS, 1, std::nullopt};
    try {
        auto time_unit_att = time_var.getAtt("units");

        // if time att is not encoded
        // TODO determine how this should be handled
        std::string time_unit_str;

        if ( !time_unit_att.isNull() )
        {
            time_unit_att.getValues(time_unit_str);
        }

        // set time unit, scale factor, and (for CF units) the reference epoch
        if ( auto parsed = interpret_time_units(time_unit_str) )
        {
            info = *parsed;
        }
        else {
            log_stream << "Warning using default time units\n";
        }
    }
    catch(const netCDF::exceptions::NcException& e){
        std::cerr<<e.what()<<std::endl;
        log_stream << "Warning: Couldn't read time unit attribute, using default time unit of Seconds\n";
    }
    assert(info.scale_factor != 0); // This should not happen.

    // fall back to the epoch_start attribute only when the units string didn't supply an epoch
    if ( !info.epoch_start_time.has_value() )
    {
        try {
            auto epoch_att = time_var.getAtt("epoch_start");

            if ( epoch_att.isNull() )
            {
                log_stream << "Warning using default epoch string\n";
            }
            else
            {
                std::string epoch_start_str;
                epoch_att.getValues(epoch_start_str);
                info.epoch_start_time = parse_epoch(epoch_start_str, "%D %T");
            }
        }
        catch(const netCDF::exceptions::NcException& e) {
            std::cerr<<e.what()<<std::endl;
            log_stream << "Warning using default epoch string\n";
        }
    }

    return info;
}

NetCDFPerFeatureDataProvider::NetCDFPerFeatureDataProvider(std::string input_path, time_t sim_start, time_t sim_end, utils::StreamHandler log_s)
    : log_stream(log_s)
    , file_path(input_path)
    , value_cache(N_EXPECTED_FORCING_VARS)
    , sim_start_date_time_epoch(sim_start)
    , sim_end_date_time_epoch(sim_end)
{
    //size_t sizep = 1073741824, nelemsp = 202481;
    //float preemptionp = 0.75;
    //nc_set_chunk_cache(sizep, nelemsp, preemptionp);

    //open the file
    nc_file = std::make_shared<netCDF::NcFile>(input_path, netCDF::NcFile::read);

    try{
        nc_file = std::make_shared<netCDF::NcFile>(input_path, netCDF::NcFile::read);
    }
    catch(const netCDF::exceptions::NcException& e){
        std::cerr<<"Error opening NetCDF file: "<<input_path<<std::endl;
        std::cerr<<e.what()<<std::endl;
        throw;
    }
    // do a quick test of the netcdf variables to ensure
    // they are readable, especially if compressed with HDF5 filters
    // This also has the added benefit of warming up the chunk cache
    test_data_is_readable();

    //nc_get_chunk_cache(&sizep, &nelemsp, &preemptionp);
    //std::cout << "Chunk cache parameters: "<<sizep<<", "<<nelemsp<<", "<<preemptionp<<std::endl;
    align_cache_with_chunks();
    //get the listing of all variables
    auto var_set = nc_file->getVars();

    // populate the ncvar and units caches...
    std::for_each(var_set.begin(), var_set.end(), [&](const auto& element)
    {
        std::string var_name = element.first;
        auto ncvar = nc_file->getVar(var_name);
        variable_names.push_back(var_name);
        ncvar_cache.emplace(var_name,ncvar);

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
        if(wkf != data_access::WellKnownFields.end()){
            native_units = native_units.empty() ? std::get<1>(wkf->second) : native_units;
            std::string can_name = std::get<0>(wkf->second); // the CSDMS name
            variable_names.push_back(can_name);
            ncvar_cache.emplace(can_name,ncvar);
            units_cache[can_name] = native_units;
        }

        units_cache[var_name] = native_units;
    });

    // read the variable ids
    auto ids = nc_file->getVar("ids"); 
    auto id_dim_count = ids.getDimCount();

    // some sanity checks
    if ( id_dim_count > 1)
    {
        Logger::logMsgAndThrowError("Provided NetCDF file has an \"ids\" variable with more than 1 dimension");
    }

    auto id_dim = ids.getDim(0);

    if (id_dim.isNull() )
    {
        Logger::logMsgAndThrowError("Provided NetCDF file has a NULL dimension for variable  \"ids\"");
    }

    auto num_ids = id_dim.getSize();
    if (num_ids <= 0){
        throw std::runtime_error("Provided NetCDF file has no features");
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

    // initialize the map of catchment-name to offset location and free the strings allocated by the C library
    size_t loc = 0;
    for_each( string_buffers.begin(), string_buffers.end(), [&](char* str)
    {
        loc_ids.push_back(str);
        id_pos[str] = loc++;
    });
    // Make sure we were able to read an actual string type
    // each id should start with "cat-"
    assert(loc_ids[0].size() > 4);;
    // correct string release
    nc_free_string(num_ids,&string_buffers[0]);

// Modified code to handle units, epoch start, and reading all time values correctly - KSL

    // Get the time variable - getVar collects all values at once and stores in memory
    // Extremely large timespans could be problematic, but for ngen use cases, this should not be a problem
    auto time_var = nc_file->getVar("Time");

    // Get the size of the time dimension
    size_t num_times = nc_file->getDim("time").getSize();

    std::vector<double> raw_time(num_times);

    try {
        time_var.getVar(raw_time.data());
    } catch(const netCDF::exceptions::NcException& e) {
        netcdf_ss << "Error reading time variable: " << e.what() << std::endl;
        LOG(netcdf_ss.str(), LogLevel::WARNING); netcdf_ss.str("");
        throw;
    }

    // read from the first catchment row to get the recorded times
    std::vector<size_t> start;
    start.push_back(0);
    start.push_back(0);
    std::vector<size_t> count;
    count.push_back(1);
    count.push_back(num_times);
    time_var.getVar(start, count, &raw_time[0]);

    // read the time metadata (unit, scale factor and reference epoch)
    TimeInfo time_info = get_time_metadata(time_var);
    time_unit = time_info.unit;

    // scale the time to account for time units and epoch_start
    // TODO make sure this happens with a FMA instruction
    time_vals.resize(raw_time.size());
    std::transform(raw_time.begin(), raw_time.end(), time_vals.begin(),
        [&](const auto& n){return n * time_info.scale_factor + time_info.epoch_start_time.value_or(0); });
        

    time_stride = time_vals[1] - time_vals[0];

    // verify the time stride
    #ifndef NCEP_OPERATIONS
    for (size_t i = 1; i < time_vals.size(); ++i) {
        double interval = time_vals[i] - time_vals[i-1];
        if (std::abs(interval - time_stride) > 1e-6) {
            netcdf_ss<< "Inconsistent interval at index " << i << ": " << interval << std::endl;
            LOG(netcdf_ss.str(), LogLevel::SEVERE); netcdf_ss.str("");
            netcdf_ss << "Error: Time intervals are not constant in forcing file\n" << std::endl;
            log_stream << netcdf_ss.str();
            LOG(netcdf_ss.str(), LogLevel::WARNING); netcdf_ss.str("");
            Logger::logMsgAndThrowError("Time intervals in forcing file are not constant");
        }
    }
    #endif

    netcdf_ss << "All time intervals are constant within tolerance." << std::endl;
    LOG(netcdf_ss.str(), LogLevel::SEVERE); netcdf_ss.str("");

    // determine start_time and stop_time;
    start_time = time_vals[0];
    stop_time = time_vals.back() + time_stride;

    sim_to_data_time_offset = sim_start_date_time_epoch - start_time;
}

void NetCDFPerFeatureDataProvider::hint_shared_provider_id(const std::string& id)
{
    hinted_ids.emplace(id);
}

void NetCDFPerFeatureDataProvider::maybe_update_chunks_with_hints()
{
    auto ids = hinted_ids;
    if (hinted_ids.size() == 0){
        return;
    }

    // Base cases covered in other ctor
    if (ids.size() == get_ids().size() || ids.size() == 0) {
        assert(chunks.size() == 1);
        hinted_ids.clear();
        return;
    }
    // get rid of "default" chunks, we will build them here
    chunks.clear();

    // Map from nc cat-id index to cat-id; sorted by index position
    std::map<std::size_t, std::string> idx_map;

    // remove "id_pos" keys that are not in "ids"
    {
        auto it = id_pos.begin();
        while (it != id_pos.end()) {
            auto sub = ids.find(it->first);
            if (sub != ids.end()) {
                // Here we know the id AND its position in the index
                idx_map.emplace(it->second, it->first);
                ++it;
            } else {
                it = id_pos.erase(it);
            }
        }
    }
    if(idx_map.empty()){
        throw std::runtime_error("NetCDF source has no ids matching the domain hinted ids.");
    }
    // Build chunks where a chunk has:
    // a starting nc index
    // the length of the chunk relative to its starting index
    //
    // While building the chunks, rebase nc indices to now "internal" cache indices
    auto it = idx_map.begin();
    std::string& key = it->second;

    std::size_t left, right;
    left = it->first;
    right = it->first;
    //  start nc idx, length
    std::pair<size_t, size_t> pair(left, 1);

    std::size_t n = 0;
    id_pos[key] = n;
    n++;

    // NOTE: not sure if there are dependencies elsewhere on the ordering of this vector.
    // refill in the expected order just to be on the safe side.
    loc_ids.clear();
    loc_ids.push_back(key);
    for (++it; it != idx_map.end(); ++it) {
        std::size_t current = it->first;
        if (right < current-1){
            pair.second = right-left+1;
            chunks.push_back(pair);

            left  = current;
            right = current;
            pair.first = current;
        }else{
            right = current;
        }

        key = it->second;
        // NOTE: update "id_pos" with new "internal" index
        id_pos[key] = n;
        n++;

        // push key back onto "loc_ids" in its original order
        loc_ids.push_back(key);
    }
    pair.second = right-left+1;
    chunks.push_back(pair);

    // minor gains
    loc_ids.shrink_to_fit();

    cache_slice_c_size = loc_ids.size();

    // TODO: improve this; we only want this method to "do something" once.
    //       invariant is, weakly, enforced by prelude guard clause.
    hinted_ids.clear();
}

NetCDFPerFeatureDataProvider::~NetCDFPerFeatureDataProvider() = default;

void NetCDFPerFeatureDataProvider::finalize()
{
    if (nc_file != nullptr) {
        nc_file->close();
    }
    nc_file = nullptr;
}

boost::span<const std::string> NetCDFPerFeatureDataProvider::get_available_variable_names() const
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
        ss << "The value " << (int)epoch_time << " was not in the range [" << (int)start_time << "," << (int)stop_time << ")\n" << SOURCE_LOC;
        LOG(ss.str(), LogLevel::WARNING);
        throw std::out_of_range(ss.str().c_str());
    }
}

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

    auto init_time = selector.get_init_time();
    auto stop_time = init_time + selector.get_duration_secs();  // scope hiding! BAD JUJU!
    
    size_t c_idx1 = get_ts_index_for_time(init_time);
    size_t c_idx2;
    try {
        c_idx2 = get_ts_index_for_time(stop_time-1); // Don't include next timestep when duration % timestep = 0
    }
    catch(const std::out_of_range &e){
        c_idx2 = get_ts_index_for_time(this->stop_time-1); //to the edge
    }

    // update chunks during the first timestep
    if (hinted_ids.size() > 0){
        // 'maybe_update_chunks_with_hints' clears 'hinted_ids'
        // assumes all id's will have been hinted before 'get_value' is called.
        maybe_update_chunks_with_hints();
        netcdf_ss << "Warning: stop_time out of range, using last available time index" << std::endl;
        log_stream << netcdf_ss.str();
        LOG(netcdf_ss.str(), LogLevel::SEVERE); netcdf_ss.str("");
        c_idx2 = get_ts_index_for_time(this->stop_time-1); //to the edge
    }

    auto stride = c_idx2 - c_idx1;

    std::vector<std::size_t> start, count;

    auto i_idx = id_pos[selector.get_id()];

    double t1 = time_vals[c_idx1];
    double t2 = time_vals[c_idx2];

    double rvalue = 0.0;
    
    auto ncvar = get_ncvar(selector.get_variable_name());

    std::string native_units = get_ncvar_units(selector.get_variable_name());

    const std::size_t read_len = c_idx2 - c_idx1 + 1;

    std::vector<double> raw_values;
    raw_values.reserve(read_len);

    std::size_t cache_line_size = cache_slice_t_size;
    std::size_t p_idx = cache::page_p_idx(c_idx1, cache_line_size);
    // pages spanned by [c_idx1, c_idx2]; ceil(read_len / line_size) undercounts
    // when the range starts mid-page and crosses a page boundary
    std::size_t n_page_accesses = cache::page_p_idx(c_idx2, cache_line_size) - p_idx + 1;

    std::size_t c_idx = c_idx1;
    // For reference: https://stackoverflow.com/a/72030286
    for( size_t i = 0; i < n_page_accesses; i++ ) {
        // rows: catchments; columns: time;
        // stride between rows is 'cache_line_size'
        std::shared_ptr<std::vector<double>> cached;

	std::size_t ith_p_idx = p_idx + i;
	std::size_t page_c_idx = cache::page_p_idx_to_c_idx(ith_p_idx, cache_line_size);
	std::size_t page_cache_line_size = cache::page_cache_line_size(page_c_idx, time_vals.size(), cache_line_size);

        std::string key = ncvar.getName() + "|" + std::to_string(page_c_idx);
        if(value_cache.contains(key)){
            cached = value_cache.get(key).get();
        } else {
            cached = std::make_shared<std::vector<double>>(get_ids().size() * page_cache_line_size);

            // read each chunk and add it to "cached"
            std::size_t idx = 0;
            for(auto const& chunk: chunks){
                // chunk start index = chunk.first;
                // chunk length      = chunk.second;
                start.clear();
                start.push_back(chunk.first);

                // NOTE: in the first iteration, we might read more data in the Time
                // dimension than we 'need'. b.c. we read from:
                // 'c_idx1 - (c_idx1 % cache_slice_t_size)' to the end of the cache line.
                // so, if 'c_idx1 % cache_slice_t_size > 0' we will read
                // 'c_idx1 % cache_slice_t_size * next_chunk_idx' more values than we 'need' to.
                start.push_back(page_c_idx);

                count.clear();
                count.push_back(chunk.second);

                count.push_back(page_cache_line_size);
                ncvar.getVar(start,count,&(*cached)[idx]);
                idx += chunk.second * page_cache_line_size;
            }

            value_cache.insert(key, cached);
        }
        // Find all values in the current cache slice and push them onto raw_values
        while(c_idx >= page_c_idx &&
              c_idx < page_c_idx + page_cache_line_size &&
              c_idx <= c_idx2){
            std::size_t idx = cache::page_entry_idx(i_idx, c_idx, time_vals.size(), cache_line_size);
            double value = cached->at(idx);
            raw_values.push_back(value);
            c_idx++;
        }
    }

    assert(raw_values.size() == read_len);
    rvalue = 0.0;

    double a , b = 0.0;
    
    a = 1.0 - ( (t1 - init_time) / time_stride );
    rvalue += (a * raw_values[0]);

    for( size_t i = 1; i < raw_values.size() -1; ++i )
    {
        rvalue += raw_values[i];
    }

    if (  raw_values.size() > 1) // likewise the last data value may not be fully in the window
    {
        b = (stop_time - t2) / time_stride;
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

    try 
    {
        //minor change to aid debugging
        double converted_value = UnitsHelper::get_converted_value(native_units, rvalue, selector.get_output_units());
        return converted_value;
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

const netCDF::NcVar& NetCDFPerFeatureDataProvider::get_ncvar(const std::string& name){
    auto cache_hit = ncvar_cache.find(name);
    if(cache_hit != ncvar_cache.end()){
        return cache_hit->second;
    }

    std::string throw_msg; throw_msg.assign("Got request for variable " + name + " but it was not found in the cache. This should not happen." + SOURCE_LOC);
    LOG(throw_msg, LogLevel::WARNING);
    throw std::runtime_error(throw_msg);
}

const std::string& NetCDFPerFeatureDataProvider::get_ncvar_units(const std::string& name){
    auto cache_hit = units_cache.find(name);
    if(cache_hit != units_cache.end()){
        return cache_hit->second;
    }

    std::string throw_msg; throw_msg.assign("Got units request for variable " + name + " but it was not found in the cache. This should not happen." + SOURCE_LOC);
    LOG(throw_msg, LogLevel::WARNING);
    throw std::runtime_error(throw_msg);

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
            } else if (var.getType().getTypeClass() == NC_INT) {
                // Handle int variable
                int val;
                var.getVar(start, count, &val);
            } else if (var.getType().getTypeClass() == NC_DOUBLE) {
                // Handle double variable
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
    auto num_times = nc_file->getDim("time").getSize();

    auto var_set = nc_file->getVars();
    for (const auto& var_pair : var_set) {
        const std::string& var_name = var_pair.first;
        auto var = var_pair.second;
        // Skip the Time variable itself and ids, look for data variables with at least 2 dimensions
        if (var_name == "Time" || var_name == "ids" || var_name == "catchment-id" || var.getDimCount() < 2) continue;
        netCDF::NcVar::ChunkMode mode;
        std::vector<size_t> chunk_sizes;
        var.getChunkingParameters(mode, chunk_sizes);
        if (mode == netCDF::NcVar::ChunkMode::nc_CHUNKED && chunk_sizes.size() >= 2) {
            cache_slice_t_size = chunk_sizes[1];  // Assume second dimension is time
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
