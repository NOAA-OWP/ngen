#include <NGenConfig.h>

#if NGEN_WITH_NETCDF
#include "NetCDFPerFeatureDataProvider.hpp"

#include <netcdf>

std::mutex data_access::NetCDFPerFeatureDataProvider::shared_providers_mutex;
std::map<std::string, std::shared_ptr<data_access::NetCDFPerFeatureDataProvider>> data_access::NetCDFPerFeatureDataProvider::shared_providers;

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

NetCDFPerFeatureDataProvider::NetCDFPerFeatureDataProvider(std::string input_path, time_t sim_start, time_t sim_end,  utils::StreamHandler log_s) : log_stream(log_s), value_cache(20),
    sim_start_date_time_epoch(sim_start),
    sim_end_date_time_epoch(sim_end)
{
    //size_t sizep = 1073741824, nelemsp = 202481;
    //float preemptionp = 0.75;
    //nc_set_chunk_cache(sizep, nelemsp, preemptionp);

    //open the file
    nc_file = std::make_shared<netCDF::NcFile>(input_path, netCDF::NcFile::read);
    
    //nc_get_chunk_cache(&sizep, &nelemsp, &preemptionp);
    //std::cout << "Chunk cache parameters: "<<sizep<<", "<<nelemsp<<", "<<preemptionp<<std::endl;

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
        throw std::runtime_error("Provided NetCDF file has an \"ids\" variable with more than 1 dimension");       
    }

    auto id_dim = ids.getDim(0);

    if (id_dim.isNull() )
    {
        throw std::runtime_error("Provided NetCDF file has a NuLL dimension for variable  \"ids\"");
    }

    auto num_ids = id_dim.getSize();

    //TODO: split into smaller slices if num_ids is large.
    cache_slice_c_size = num_ids;

    // allocate an array of character pointers 
    std::vector< char* > string_buffers(num_ids);

    // read the id strings
    ids.getVar(&string_buffers[0]);

    // initalize the map of catchment-name to offset location and free the strings allocated by the C library
    size_t loc = 0;
    for_each( string_buffers.begin(), string_buffers.end(), [&](char* str)
    {
        loc_ids.push_back(str);
        id_pos[str] = loc++;
    });

    // correct string release
    nc_free_string(num_ids,&string_buffers[0]);

    // now get the size of the time dimension
    auto num_times = nc_file->getDim("time").getSize();

    // allocate storage for the raw time array
    std::vector<double> raw_time(num_times);

    // get the time variable
    auto time_var = nc_file->getVar("Time");

    // read from the first catchment row to get the recorded times
    std::vector<size_t> start;
    start.push_back(0);
    start.push_back(0);
    std::vector<size_t> count;
    count.push_back(1);
    count.push_back(num_times);
    time_var.getVar(start, count, &raw_time[0]);

    // read the meta data to get the time_unit
    // if absent, assume seconds
    double time_scale_factor = 1;
    time_unit = TIME_SECONDS;
    try {
        auto time_unit_att = time_var.getAtt("units");

        // if time att is not encoded 
        // TODO determine how this should be handled
        std::string time_unit_str;

        if ( time_unit_att.isNull() )
        {
            log_stream << "Warning using defualt time units\n";
        }
        else
        {  
            time_unit_att.getValues(time_unit_str);
        }

        // set time unit and scale factor
        if ( time_unit_str == "h" || time_unit_str == "hours")
        {
            time_unit = TIME_HOURS;
            time_scale_factor = 3600;
        }
        else if ( time_unit_str == "m" || time_unit_str == "minutes" )
        {
            time_unit = TIME_MINUTES;
            time_scale_factor = 60;
        }
        else if ( time_unit_str ==  "s" || time_unit_str == "seconds" )
        {
            time_unit = TIME_SECONDS;
            time_scale_factor = 1;
        }
        else if ( time_unit_str ==  "ms" || time_unit_str == "milliseconds" )
        {
            time_unit = TIME_MILLISECONDS;
            time_scale_factor = .001;
        }
        else if ( time_unit_str ==  "us" || time_unit_str == "microseconds" )
        {
            time_unit = TIME_MICROSECONDS;
            time_scale_factor = .000001;
        }
        else if ( time_unit_str ==  "ns" || time_unit_str == "nanoseconds" )
        {
            time_unit = TIME_NANOSECONDS;
            time_scale_factor = .000000001;
        }
        else {
            log_stream << "Warning using defualt time units\n";
        }
    }
    catch(const netCDF::exceptions::NcException& e){
        std::cerr<<e.what()<<std::endl;
        log_stream << "Warning using defualt time units\n";
    }
    assert(time_scale_factor != 0); // This should not happen.

    std::string epoch_start_str = "01/01/1970 00:00:00";
    try {
        // read the meta data to get the epoc start
        auto epoch_att = time_var.getAtt("epoch_start");

        if ( epoch_att.isNull() )
        {
            log_stream << "Warning using defualt epoc string\n";
        }
        else
        {  
            epoch_att.getValues(epoch_start_str);
        }
    }
    catch(const netCDF::exceptions::NcException& e) {
        std::cerr<<e.what()<<std::endl;
        log_stream << "Warning using defualt epoc string\n";
    }
    
    std::tm tm{};
    std::stringstream s(epoch_start_str);
    s >> std::get_time(&tm, "%D %T");
    //std::time_t epoch_start_time = mktime(&tm);
    // See also comments in Simulation_Time.h .. timegm is not available on Windows at least (elsewhere?)
    //TODO: Probably make the default string above explicit to UTC and interpret TZ from the string in all cases?
    std::time_t epoch_start_time = timegm(&tm);

    // scale the time to account for time units and epoch_start
    // TODO make sure this happens with a FMA instruction
    time_vals.resize(raw_time.size());
    std::transform(raw_time.begin(), raw_time.end(), time_vals.begin(), 
        [&](const auto& n){return n * time_scale_factor + epoch_start_time; });
        

    // determine the stride of the time array
    time_stride = time_vals[1] - time_vals[0];

    #ifndef NCEP_OPERATIONS
    // verify the time stride
    for( size_t i = 1; i < time_vals.size() -1; ++i)
    {
        double tinterval = time_vals[i+1] - time_vals[i];

        if ( tinterval - time_stride > 0.000001)
        {
            log_stream << "Error: Time intervals are not constant in forcing file\n";

            throw std::runtime_error("Time intervals in forcing file are not constant");
        }
    }
    #endif

    // determine start_time and stop_time;
    start_time = time_vals[0];
    stop_time = time_vals.back() + time_stride;

    sim_to_data_time_offset = sim_start_date_time_epoch - start_time;
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
        throw std::out_of_range(ss.str().c_str());
    }
}

double NetCDFPerFeatureDataProvider::get_value(const CatchmentAggrDataSelector& selector, ReSampleMethod m) 
{
    auto init_time = selector.get_init_time();
    auto stop_time = init_time + selector.get_duration_secs(); // scope hiding! BAD JUJU!
    
    size_t idx1 = get_ts_index_for_time(init_time);
    size_t idx2;
    try {
        idx2 = get_ts_index_for_time(stop_time-1); // Don't include next timestep when duration % timestep = 0
    }
    catch(const std::out_of_range &e){
        idx2 = get_ts_index_for_time(this->stop_time-1); //to the edge
    }

    auto stride = idx2 - idx1;

    std::vector<std::size_t> start, count;

    auto cat_pos = id_pos[selector.get_id()];



    double t1 = time_vals[idx1];
    double t2 = time_vals[idx2];

    double rvalue = 0.0;
    
    auto ncvar = get_ncvar(selector.get_variable_name());

    std::string native_units = get_ncvar_units(selector.get_variable_name());

    auto read_len = idx2 - idx1 + 1;

    std::vector<double> raw_values;
    raw_values.resize(read_len);

    //TODO: Currently assuming a whole variable cache slice across all catchments for a single timestep...but some stuff here to support otherwise.
    size_t cache_slices_t_n = read_len / cache_slice_t_size; // Integer division!
    // For reference: https://stackoverflow.com/a/72030286
    for( size_t i = 0; i < cache_slices_t_n; i++ ) {
        std::shared_ptr<std::vector<double>> cached;
        int cache_t_idx = (idx1 - (idx1 % cache_slice_t_size) + i);
        std::string key = ncvar.getName() + "|" + std::to_string(cache_t_idx);
        if(value_cache.contains(key)){
            cached = value_cache.get(key).get();
        } else {
            cached = std::make_shared<std::vector<double>>(cache_slice_c_size * cache_slice_t_size);
            start.clear();
            start.push_back(0); // only always 0 when cache_slice_c_size = numids!
            start.push_back(cache_t_idx * cache_slice_t_size);
            count.clear();
            count.push_back(cache_slice_c_size);
            count.push_back(cache_slice_t_size); // Must be 1 for now!...probably...
            ncvar.getVar(start,count,&(*cached)[0]);
            value_cache.insert(key, cached);
        }
        for( size_t j = 0; j < cache_slice_t_size; j++){
            raw_values[i+j] = cached->at((j*cache_slice_t_size) + cat_pos);
        }
    }

    
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
        case SUM:   // we allready have the sum so do nothing
            ;
        break;

        case MEAN: 
        { 
            // This is getting a length weighted mean
            // the data values where allready scaled for where there was only partial use of a data value
            // so we just need to do a final scale to account for the differnce between time_stride and duration_s

            double scale_factor = (selector.get_duration_secs() > time_stride ) ? (time_stride / selector.get_duration_secs()) : (1.0 / (a + b));
            rvalue *= scale_factor;
        }
        break;

        default:
            ;
    }

    try 
    {
        return UnitsHelper::get_converted_value(native_units, rvalue, selector.get_output_units());
    }
    catch (const std::runtime_error& e)
    {
        #ifndef UDUNITS_QUIET
        std::cerr<<"WARN: Unit conversion unsuccessful - Returning unconverted value! (\""<<e.what()<<"\")"<<std::endl;
        #endif
        return rvalue;
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

    throw std::runtime_error("Got request for variable " + name + " but it was not found in the cache. This should not happen." + SOURCE_LOC);
}

const std::string& NetCDFPerFeatureDataProvider::get_ncvar_units(const std::string& name){
    auto cache_hit = units_cache.find(name);
    if(cache_hit != units_cache.end()){
        return cache_hit->second;
    }

    throw std::runtime_error("Got units request for variable " + name + " but it was not found in the cache. This should not happen." + SOURCE_LOC);
}

}

#endif
