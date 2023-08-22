#ifndef NGEN_UTILITIES_QLAT_HANDLER_HPP
#define NGEN_UTILITIES_QLAT_HANDLER_HPP

#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <vector>

#ifdef NETCDF_ACTIVE
#include <netcdf>
#endif

#define NGEN_NETCDF_ERROR throw std::runtime_error("NetCDF is not available")

// Changing these will affect both NetCDF and CSV outputs
#define NGEN_VARIABLE_NEXUS_ID   "feature_id"
#define NGEN_VARIABLE_SEGMENT_ID "segment_id"
#define NGEN_VARIABLE_Q_LATERAL  "q_lateral"

namespace ngen {
namespace utils {

/**
 * Abstract nexus output writer
 */
struct nexus_writer
{   
    using size_type = size_t;

    nexus_writer() = default;

    virtual ~nexus_writer() = default;

    /**
     * @brief Initialize this writer
     * 
     * @param n Number of time steps
     */
    virtual void init(size_type steps) = 0;

    /**
     * @brief Iterate to the next file.
     * 
     * @param file_name File path to create
     * @param num_nexuses Number of nexuses contained in this output
     */
    virtual void next(const std::string& file_name, size_type num_nexuses) = 0;

    /**
     * @brief Write a line to the current file.
     * 
     * @param segment_id Flowpath/segment ID
     * @param nexus_id Nexus ID
     * @param qlat Lateral streamflow
     */
    virtual void write(const std::string& segment_id, const std::string& nexus_id, double qlat) = 0;

    /**
     * @brief Flush current output
     * 
     * @note For NetCDF this should close the file.
     */
    virtual void flush() = 0;
};

/**
 * @brief CSV derived nexus writer
 */
struct nexus_csv_writer : public nexus_writer
{
    using size_type = size_t;

    nexus_csv_writer() = default;

    ~nexus_csv_writer() override = default;

    void init(size_type steps) override
    {
        outputs_.resize(steps);
        it_ = outputs_.begin();
        it_--;
    }

    void next(const std::string& file_name, size_type num_nexuses) override
    {
        it_++;
        it_->open(file_name + ".csv");
        (*it_) << NGEN_VARIABLE_NEXUS_ID "," NGEN_VARIABLE_SEGMENT_ID "," NGEN_VARIABLE_Q_LATERAL "\n";
    }

    void write(const std::string& segment_id, const std::string& nexus_id, double contribution) override
    {
        (*it_) << nexus_id << ',' << segment_id << ',' << std::setprecision(14) << contribution << '\n';
    }

    void flush() override
    {
        it_->flush();
    }

  private:
    std::vector<std::ofstream> outputs_;
    decltype(outputs_)::iterator it_;
};

/**
 * @brief NetCDF derived nexus writer
 * @note If NetCDF is not available, the methods of this struct will throw a std::runtime_error.
 */
struct nexus_netcdf_writer : public nexus_writer
{
    using size_type = size_t;

    nexus_netcdf_writer() = default;

    ~nexus_netcdf_writer() override = default;

    void init(size_type steps) override
    {
#ifndef NETCDF_ACTIVE
        NGEN_NETCDF_ERROR;
#else
        outputs_.resize(steps);
        it_ = outputs_.begin();
        it_--;
#endif
    }

    void next(const std::string& file_name, size_type num_nexuses) override
    {
#ifndef NETCDF_ACTIVE
        NGEN_NETCDF_ERROR;
#else
        it_++;
        std::unique_ptr<netCDF::NcFile>& out = *it_;

        out = std::make_unique<netCDF::NcFile>(file_name + ".nc", netCDF::NcFile::replace);
        const auto& dim_fid = out->addDim("feature_id", num_nexuses);
  
        const auto& var_nexus_id = out->addVar(NGEN_VARIABLE_NEXUS_ID, netCDF::ncString, dim_fid);
        var_nexus_id.putAtt("description", "Contributing Nexus ID");
        
        const auto& var_segment_id = out->addVar(NGEN_VARIABLE_SEGMENT_ID, netCDF::ncString, dim_fid);
        var_segment_id.putAtt("description", "Flowpath ID downstream from the corresponding nexus");

        const auto& var_qlat = out->addVar(NGEN_VARIABLE_Q_LATERAL, netCDF::ncDouble, dim_fid);
        var_qlat.putAtt("description", "Runoff into channel reach");
        var_qlat.putAtt("units", "m3 s-1");
    
        index_ = 0;
#endif
    }

    void write(const std::string& segment_id, const std::string& nexus_id, double contribution) override
    {
#ifndef NETCDF_ACTIVE
        NGEN_NETCDF_ERROR;
#else
        it_->get()->getVar(NGEN_VARIABLE_NEXUS_ID).putVar({ index_ }, nexus_id);
        it_->get()->getVar(NGEN_VARIABLE_SEGMENT_ID).putVar({ index_ }, segment_id);
        it_->get()->getVar(NGEN_VARIABLE_Q_LATERAL).putVar({ index_ }, contribution);
        index_++;
#endif
    }

    void flush() override
    {
#ifndef NETCDF_ACTIVE
        NGEN_NETCDF_ERROR;
#else
        it_->get()->close();
#endif
    }

  private:
#ifdef NETCDF_ACTIVE
    std::vector<std::unique_ptr<netCDF::NcFile>> outputs_;
    decltype(outputs_)::iterator it_;
    size_type index_ = 0;
#endif
};

}
}

#endif // NGEN_UTILITIES_QLAT_HANDLER_HPP

#undef NGEN_NETCDF_ERROR
#undef NGEN_VARIABLE_NEXUS_ID
#undef NGEN_VARIABLE_SEGMENT_ID
#undef NGEN_VARIABLE_Q_LATERAL
