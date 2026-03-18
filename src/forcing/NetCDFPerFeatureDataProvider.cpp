#include <NGenConfig.h>

#if NGEN_WITH_NETCDF
#include "NetCDFPerFeatureDataProvider.hpp"

#include <netcdf>

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

NetCDFPerFeatureDataProvider::NetCDFPerFeatureDataProvider(std::string input_path, time_t sim_start, time_t sim_end,  utils::StreamHandler log_s) : log_stream(log_s), value_cache(N_EXPECTED_FORCING_VARS),
    sim_start_date_time_epoch(sim_start),
    sim_end_date_time_epoch(sim_end)
{
    //size_t sizep = 1073741824, nelemsp = 202481;
    //float preemptionp = 0.75;
    //nc_set_chunk_cache(sizep, nelemsp, preemptionp);

    //open the file
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
        throw std::runtime_error("Provided NetCDF file has an \"ids\" variable with more than 1 dimension");       
    }

    auto id_dim = ids.getDim(0);

    if (id_dim.isNull() )
    {
        throw std::runtime_error("Provided NetCDF file has a NuLL dimension for variable  \"ids\"");
    }

    auto num_ids = id_dim.getSize();
    assert(num_ids > 0);
    // include all catchments in the "default" chunk
    auto pair = std::pair<size_t, size_t>(0, num_ids);
    chunks.push_back(pair);

    //TODO: split into smaller slices if num_ids is large.
    cache_slice_c_size = num_ids;

    // allocate an array of character pointers 
    std::vector< char* > string_buffers(num_ids);

    // read the id strings
    ids.getVar(string_buffers.data());
    // initalize the map of catchment-name to offset location and free the strings allocated by the C library
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
    std::string unit_epoch_str = "";
    try {
        auto time_unit_att = time_var.getAtt("units");

        // if time att is not encoded 
        // TODO determine how this should be handled
        std::string time_unit_str;

        if ( !time_unit_att.isNull() )
        {
            time_unit_att.getValues(time_unit_str);
        }

        // CF conventions may have units of the form "<unit> since <date>"
        size_t pos = time_unit_str.find(" since ");
        if(pos != std::string::npos)
        {
            unit_epoch_str = time_unit_str.substr(pos + 7); // 7 is length of " since "
            time_unit_str.erase(pos);
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
        log_stream << "Warning: Couldn't read time unit attribute, using defualt time unit of Seconds\n";
    }
    assert(time_scale_factor != 0); // This should not happen.

    std::string epoch_start_str = "01/01/1970 00:00:00";
    std::string epoch_format = "%D %T";
    if(!unit_epoch_str.empty())
    {
        epoch_start_str = unit_epoch_str;
        //CF convention in time units formats as "YYYY-MM-DD HH:MM:SS"
        epoch_format = "%Y-%m-%d %H:%M:%S";
    }
    else{
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
    }
    std::tm tm{};
    std::stringstream s(epoch_start_str);
    s >> std::get_time(&tm, epoch_format.c_str());
    if(s.fail()){
        std::stringstream ss;
        ss << "std::get_time failed to parse epoch string with: " << epoch_start_str << " with format string " << epoch_format << "\n";
        log_stream << ss.str();
        throw std::runtime_error(ss.str());
    }
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

    // update chunks during the first timestep
    if (hinted_ids.size() > 0){
        // 'maybe_update_chunks_with_hints' clears 'hinted_ids'
        // assumes all id's will have been hinted before 'get_value' is called.
        maybe_update_chunks_with_hints();
    }

    auto stride = idx2 - idx1;

    std::vector<std::size_t> start, count;

    auto cat_idx = id_pos[selector.get_id()];

    double t1 = time_vals[idx1];
    double t2 = time_vals[idx2];

    double rvalue = 0.0;
    
    auto ncvar = get_ncvar(selector.get_variable_name());

    std::string native_units = get_ncvar_units(selector.get_variable_name());

    const std::size_t read_len = idx2 - idx1 + 1;

    std::vector<double> raw_values;
    raw_values.reserve(read_len);

    std::size_t idx1_cache_slice_start = idx1 - (idx1 % cache_slice_t_size);
    std::size_t time_idx = idx1 % cache_slice_t_size;

    std::size_t n_cache_slices = (read_len / cache_slice_t_size) + std::min(read_len % cache_slice_t_size, std::size_t(1));
    size_t value_idx = idx1;
    // For reference: https://stackoverflow.com/a/72030286
    for( size_t i = 0; i < n_cache_slices; i++ ) {
        // rows: catchments; columns: time;
        // stride between rows is 'cache_slice_t_size'
        std::shared_ptr<std::vector<double>> cached;

        std::size_t cache_slice_start = idx1_cache_slice_start + (i * cache_slice_t_size);
        std::size_t adjusted_size = cache_slice_t_size;
        // Read up to the end of the data, but not beyond
        if(cache_slice_start + cache_slice_t_size > time_vals.size() ){
            adjusted_size = time_vals.size() - cache_slice_start;
        }
        std::string key = ncvar.getName() + "|" + std::to_string(cache_slice_start);
        if(value_cache.contains(key)){
            cached = value_cache.get(key).get();
        } else {
            // NOTE: aaraney: I am leaning towards just 'over allocating'
            /* size_t last_bucket = get_ts_index_for_time(this->stop_time-1) / bucket_size; */
            /* assert(bucket_idx <= last_bucket); */
            /* size_t n_time = bucket_size; */
            /* if (bucket_idx == last_bucket){ */
            /*     n_time = (get_ts_index_for_time(this->stop_time-1) % bucket_size) + 1; */
            /* } */
            cached = std::make_shared<std::vector<double>>(get_ids().size() *  cache_slice_t_size);

            // read each chunk and add it to "cached"
            std::size_t next_cache_idx = 0;
            for(auto const& chunk: chunks){
                // chunk start index = chunk.first;
                // chunk length      = chunk.second;
                start.clear();
                start.push_back(chunk.first);

                // NOTE: in the first iteration, we might read more data in the Time
                // dimension than we 'need'. b.c. we read from:
                // 'idx1 - (idx1 % cache_slice_t_size)' to the end of the cache line.
                // so, if 'idx1 % cache_slice_t_size > 0' we will read
                // 'idx1 % cache_slice_t_size * next_chunk_idx' more values than we 'need' to.
                start.push_back(cache_slice_start);

                count.clear();
                count.push_back(chunk.second);

                count.push_back(adjusted_size);
                ncvar.getVar(start,count,&(*cached)[next_cache_idx]);
                next_cache_idx += chunk.second * adjusted_size;
            }

            value_cache.insert(key, cached);
        }
        // Find all values in the current cache slice and push them onto raw_values
        while(value_idx >= cache_slice_start && 
              value_idx < cache_slice_start + cache_slice_t_size &&
              value_idx <= idx2){
            raw_values.push_back(cached->at((cat_idx * cache_slice_t_size) + (value_idx % cache_slice_t_size)));
            value_idx++;
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
            // libnetcxx puts a lot of noice in the error message
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
    if(cache_slice_t_size > num_times){
        cache_slice_t_size = num_times;
    }else if(cache_slice_t_size <= 0){
        cache_slice_t_size = 24; // default to 24 if no chunking info found
    }
    //At this point the time slice cache is either aligned with the chunking of the data variables
    //or set to a default value of 24 (e.g. one day of hourly data)
    return;
}

}

#endif
