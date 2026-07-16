#ifndef NetCDFFILE_HPP
#define NetCDFFILE_HPP

//#include <NGenConfig.h>

#if NGEN_WITH_NETCDF
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <netcdf.h>

#if NGEN_WITH_MPI
#include <mpi.h>
#define _PARALLEL4
#endif
#include "NetCDFVar.hpp"


enum class NetCDFOpenMode {
    OPEN_READ,
    OPEN_WRITE,
    CREATE
};

class NetCDFFile {
public:

    NetCDFFile(const std::string& filename, NetCDFOpenMode open_mode, bool is_mpi);
    ~NetCDFFile();

    void load_attributes(std::shared_ptr<NetCDFVar> nc_var) ;
    std::shared_ptr<NetCDFVar> get_ncvar(const std::string& name) const;
    int get_ncid() const { return ncid_; }

    // Dimension handling
    int add_dimension(const std::string& name, size_t len);
    size_t get_dim_size(const std::string& name) const;
    int get_dim_id(const std::string& name) const;

    // Variable handling
    void load_variables(); 
    std::vector<std::string> list_variables() const;
    void add_ncvar(std::shared_ptr<NetCDFVar> var);
    void add_variable(const std::string& name, nc_type type, const std::vector<int>& dims, const std::vector<std::string>& dim_names);

    // Write data to a variable
    template<typename T>
    void write_variable_data(const std::string& name, const std::vector<T>& data, size_t start_index = 0);

    void write_data_to_ncvar(int ncid, int varid, const std::vector<size_t>& start, 
                 const std::vector<size_t>& count, const std::vector<double>& data);

    void write_data_to_ncvar(int ncid, int varid, const std::vector<size_t>& start, 
                 const std::vector<size_t>& count, const std::vector<int>& data);

    void write_data_to_ncvar(int ncid, int varid, const std::vector<size_t>& start, 
                 const std::vector<size_t>& count, const std::vector<int64_t>& data);

    void write_data_to_ncvar(int ncid, int varid, const std::vector<size_t>& start, 
                 const std::vector<size_t>& count, const std::vector<std::string>& data);
    
    void write_catchment_output_data(const std::string& name, std::vector<size_t> start,
                        std::vector<size_t> count, const double data);

    //Add attribute to a variable
    void write_attribute_to_ncvar(const std::string& name, const std::string& attName, const std::string& attValue);

    void close_file();

    void end_def_mode();

private:
    std::string nc_file_name_;
    bool read_only_;
    int ncid_;
    bool is_mpi_;
    std::vector<std::shared_ptr<NetCDFVar>> variables_;
    std::map<std::string, int> dims_id_map_;
    std::map<std::string, std::shared_ptr<NetCDFVar>> variables_map_;
    std::shared_ptr<NetCDFVar> get_ncvar_by_name(const std::string& name) const;

#if NGEN_WITH_MPI
    MPI_Comm comm_;
#endif
};
#endif // NGEN_WITH_NETCDF
#endif // NETCDFFILE_HPP