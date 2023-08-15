#ifndef NGEN_UTILITIES_QLAT_HANDLER_HPP
#define NGEN_UTILITIES_QLAT_HANDLER_HPP

#include <iomanip>
#include <vector>
#include <memory>

#include <fstream>

#ifdef NETCDF_ACTIVE
#include <netcdf>
#endif

#define NGEN_NETCDF_ERROR throw std::runtime_error("NetCDF is not available")

namespace ngen {
namespace utils {

struct nexus_writer
{   
    using size_type = size_t;

    nexus_writer() = default;

    virtual ~nexus_writer() {};

    virtual void init(size_type n) = 0;
    virtual void next(const std::string& file_name, size_type num_nexuses) = 0;
    virtual void write(const std::string& segment_id, const std::string& nexus_id, double qlat) = 0;
    virtual void flush() = 0;
};

struct nexus_csv_writer : public nexus_writer
{
    using size_type = size_t;

    nexus_csv_writer() = default;

    ~nexus_csv_writer() override {};

    void init(size_type n) override
    {
        outputs_.resize(n);
        it_ = outputs_.begin();
        it_--;
    }

    void next(const std::string& file_name, size_type num_nexuses) override
    {
        it_++;
        it_->open(file_name + ".csv");
        (*it_) << "nexus_id,segment_id,qSfcLatRunoff\n";
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

struct nexus_netcdf_writer : public nexus_writer
{
    using size_type = size_t;

    nexus_netcdf_writer() = default;

    ~nexus_netcdf_writer() override {};

    void init(size_type n) override
    {
#ifndef NETCDF_ACTIVE
        NGEN_NETCDF_ERROR;
#else
        outputs_.resize(n);
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
  
        const auto& var_nexus_id = out->addVar("nexus_id", netCDF::ncString, dim_fid);
        var_nexus_id.putAtt("description", "Contributing Nexus ID");
        
        const auto& var_segment_id = out->addVar("segment_id", netCDF::ncString, dim_fid);
        var_segment_id.putAtt("description", "Flowpath ID downstream from the corresponding nexus");

        const auto& var_qlat = out->addVar("qSfcLatRunoff", netCDF::ncDouble, dim_fid);
        var_qlat.putAtt("description", "Runoff from terrain routing");
        var_qlat.putAtt("units", "m3 s-1");
    
        index_ = 0;
#endif
    }

    void write(const std::string& segment_id, const std::string& nexus_id, double contribution) override
    {
#ifndef NETCDF_ACTIVE
        NGEN_NETCDF_ERROR;
#else
        std::cerr << "writing at " << index_ << '\n';
        it_->get()->getVar("nexus_id").putVar({ index_ }, nexus_id);
        it_->get()->getVar("segment_id").putVar({ index_ }, segment_id);
        it_->get()->getVar("qSfcLatRunoff").putVar({ index_ }, contribution);
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
