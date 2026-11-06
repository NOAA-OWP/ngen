#include <NGenConfig.h>

#if NGEN_WITH_NETCDF
#include "gtest/gtest.h"
#include "NetCDFPerFeatureDataProvider.hpp"
#include "StreamHandler.hpp"
#include "UnitsHelper.hpp"

#include <netcdf>

#include <cmath>
#include <cstdio>
#include <ctime>
#include <exception>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using data_access::NetCDFPerFeatureDataProvider;

// Files that store the same forcing data in different ways must read the
// same. Each test writes one storage variant of a single variable "temp"
// whose value at catchment c and timestep t is kelvin(c, t), and checks every
// value the provider returns against it, reading every catchment and also a
// hinted subset of them.
class NetCDFStorageVariantTest : public ::testing::Test {
  protected:
    enum class Type { Double, Float, Int, Short };

    static const netCDF::NcType& ncType(Type t) {
        switch (t) {
            case Type::Float:
                return netCDF::ncFloat;
            case Type::Int:
                return netCDF::ncInt;
            case Type::Short:
                return netCDF::ncShort;
            default:
                return netCDF::ncDouble;
        }
    }

    struct Variant {
        std::size_t n_cats = 5;
        std::size_t n_times = 20;
        time_t start = 1499997600; // 2017-07-14 02:00:00 UTC
        time_t stride = 3600;

        // order[pos] is the catchment stored at position pos; empty keeps 0..n_cats-1
        std::vector<std::size_t> order;
        // store variables as (time, catchment-id) rather than (catchment-id, time)
        bool time_major = false;
        std::optional<bool> time_var_time_major; // for Time; default: as time_major
        bool unlimited_time = false;

        Type type = Type::Double;
        // packing attributes; values are stored as (value - add_offset) / scale_factor,
        // rounded for integer types
        std::optional<double> scale_factor;
        std::optional<double> add_offset;
        // missing data markers, as stored (packed) values
        std::optional<double> fill_value;    // _FillValue attribute
        std::vector<double> missing_values;  // missing_value attribute, if any (double storage only)
        std::optional<double> missing_raw;   // stored at missing cells; default: the first marker above, else NaN
        std::set<std::pair<std::size_t, std::size_t>> missing; // (catchment, timestep) cells stored as missing

        std::vector<std::size_t> chunks; // chunk sizes in file dimension order; empty for the library default
        int deflate_level = 0;

        std::string units = "K";
        std::function<double(double)> from_kelvin; // to the stored units; identity if empty

        Type time_type = Type::Double;
        std::string time_units = "seconds since 1970-01-01 00:00:00";
        std::optional<std::string> epoch_start;
        std::function<double(time_t)> encode_time; // epoch seconds to a stored Time value; identity if empty

        // variables and a dimension that forcing reads don't use
        bool extra_variables = false;
    };

    static double kelvin(std::size_t cat, std::size_t t) {
        return 250.0 + cat + 0.25 * t;
    }

    void SetUp() override {
        init_unit_system(); // as ngen's main() does, for unit conversions
    }

    void TearDown() override {
        for (const auto& p : temp_files) {
            std::remove(p.c_str());
        }
    }

    std::string write(const Variant& v) {
        std::string path = std::string("./nc_variant_") +
            ::testing::UnitTest::GetInstance()->current_test_info()->name() + ".nc";
        temp_files.push_back(path);

        std::vector<std::size_t> order = v.order;
        if (order.empty()) {
            order.resize(v.n_cats);
            std::iota(order.begin(), order.end(), 0);
        }

        netCDF::NcFile out(path, netCDF::NcFile::replace, netCDF::NcFile::nc4);
        auto cat_dim = out.addDim("catchment-id", v.n_cats);
        auto time_dim = v.unlimited_time ? out.addDim("time") : out.addDim("time", v.n_times);
        auto dims_for = [&](bool time_major) {
            return time_major ? std::vector<netCDF::NcDim>{time_dim, cat_dim}
                              : std::vector<netCDF::NcDim>{cat_dim, time_dim};
        };
        auto counts_for = [&](bool time_major) {
            return time_major ? std::vector<std::size_t>{v.n_times, v.n_cats}
                              : std::vector<std::size_t>{v.n_cats, v.n_times};
        };
        // index of (file position, timestep) in a variable stored in either order
        auto at = [&](bool time_major, std::size_t pos, std::size_t t) {
            return time_major ? t * v.n_cats + pos : pos * v.n_times + t;
        };
        const bool time_var_time_major = v.time_var_time_major.value_or(v.time_major);
        std::vector<std::size_t> starts = {0, 0};

        std::vector<std::string> id_strs;
        std::vector<const char*> id_ptrs;
        for (std::size_t c : order) {
            id_strs.push_back("cat-" + std::to_string(c));
        }
        for (const auto& s : id_strs) {
            id_ptrs.push_back(s.c_str());
        }
        out.addVar("ids", netCDF::ncString, cat_dim).putVar(id_ptrs.data());

        auto time_var = out.addVar("Time", ncType(v.time_type), dims_for(time_var_time_major));
        time_var.putAtt("units", v.time_units);
        if (v.epoch_start) {
            time_var.putAtt("epoch_start", *v.epoch_start);
        }
        std::vector<double> times(v.n_cats * v.n_times);
        for (std::size_t pos = 0; pos < v.n_cats; ++pos) {
            for (std::size_t t = 0; t < v.n_times; ++t) {
                time_t epoch = v.start + t * v.stride;
                times[at(time_var_time_major, pos, t)] = v.encode_time ? v.encode_time(epoch) : double(epoch);
            }
        }
        time_var.putVar(starts, counts_for(time_var_time_major), times.data());

        auto var = out.addVar("temp", ncType(v.type), dims_for(v.time_major));
        var.putAtt("units", v.units);
        if (!v.chunks.empty()) {
            std::vector<std::size_t> chunks = v.chunks;
            var.setChunking(netCDF::NcVar::nc_CHUNKED, chunks);
        }
        if (v.deflate_level > 0) {
            var.setCompression(true, true, v.deflate_level);
        }
        if (v.scale_factor) {
            var.putAtt("scale_factor", netCDF::ncDouble, *v.scale_factor);
        }
        if (v.add_offset) {
            var.putAtt("add_offset", netCDF::ncDouble, *v.add_offset);
        }
        if (v.fill_value) {
            putTypedAtt(var, "_FillValue", *v.fill_value);
        }
        if (!v.missing_values.empty()) {
            var.putAtt("missing_value", netCDF::ncDouble, v.missing_values.size(), v.missing_values.data());
        }
        const double missing_raw = v.missing_raw ? *v.missing_raw
            : v.fill_value ? *v.fill_value
            : !v.missing_values.empty() ? v.missing_values.front()
            : std::numeric_limits<double>::quiet_NaN();

        std::vector<double> values(v.n_cats * v.n_times);
        for (std::size_t pos = 0; pos < v.n_cats; ++pos) {
            for (std::size_t t = 0; t < v.n_times; ++t) {
                const std::size_t c = order[pos];
                double x;
                if (v.missing.count({c, t})) {
                    x = missing_raw;
                } else {
                    x = v.from_kelvin ? v.from_kelvin(kelvin(c, t)) : kelvin(c, t);
                    if (v.scale_factor || v.add_offset) {
                        x = (x - v.add_offset.value_or(0.0)) / v.scale_factor.value_or(1.0);
                        if (v.type == Type::Int || v.type == Type::Short) {
                            x = std::round(x);
                        }
                    }
                }
                values[at(v.time_major, pos, t)] = x;
            }
        }
        var.putVar(starts, counts_for(v.time_major), values.data());

        if (v.extra_variables) {
            // A coordinate, a scalar, and a variable over another dimension;
            // "bounds" is chunked and sorts before "temp"
            auto nv_dim = out.addDim("nv", 2);
            auto x_var = out.addVar("x", netCDF::ncDouble, cat_dim);
            std::vector<double> xs(v.n_cats, -82.0);
            x_var.putVar(xs.data());
            out.addVar("crs", netCDF::ncChar).putAtt("grid_mapping_name", "latitude_longitude");
            auto bounds = out.addVar("bounds", netCDF::ncDouble, std::vector<netCDF::NcDim>{time_dim, nv_dim});
            std::vector<std::size_t> bchunks = {1, 2};
            std::vector<std::size_t> bstart = {0, 0};
            std::vector<std::size_t> bcount = {v.n_times, 2};
            bounds.setChunking(netCDF::NcVar::nc_CHUNKED, bchunks);
            std::vector<double> bvals(2 * v.n_times, 0.0);
            bounds.putVar(bstart, bcount, bvals.data());
            out.putAtt("model_configuration", "short_range");
        }
        return path;
    }

    // Reads every value of the hinted catchments (all of them if none are
    // hinted) one timestep at a time, and multi-timestep means, comparing
    // them with what was written
    void expectReads(const std::string& path, const Variant& v, const std::vector<std::size_t>& hinted = {}) {
        std::vector<std::size_t> cats = hinted;
        if (cats.empty()) {
            cats.resize(v.n_cats);
            std::iota(cats.begin(), cats.end(), 0);
        }
        std::ostringstream context;
        context << path << (hinted.empty() ? " (all catchments)" : " (hinted subset)");
        SCOPED_TRACE(context.str());

        std::optional<NetCDFPerFeatureDataProvider> provider;
        try {
            provider.emplace(path, v.start, v.start + time_t(v.n_times) * v.stride, utils::getStdErr());
        } catch (const std::exception& e) {
            FAIL() << "constructing the provider threw: " << e.what();
        }
        for (std::size_t c : hinted) {
            provider->hint_shared_provider_id("cat-" + std::to_string(c));
        }

        // float storage rounds values that aren't exactly representable
        const double tolerance = v.type == Type::Float ? 1e-4 : 1e-9;
        std::size_t mismatches = 0;
        std::ostringstream first;
        first.precision(17);
        auto check = [&](std::size_t c, std::size_t t0, std::size_t n, double expected) {
            CatchmentAggrDataSelector sel("cat-" + std::to_string(c), "temp",
                                          v.start + time_t(t0) * v.stride, time_t(n) * v.stride, "K");
            double got;
            try {
                got = provider->get_value(sel, data_access::MEAN);
            } catch (const std::exception& e) {
                if (mismatches++ < 3) {
                    first << "\n  cat-" << c << " t=" << t0 << " n=" << n << ": threw " << e.what();
                }
                return;
            }
            bool ok = std::isnan(expected) ? std::isnan(got) : std::abs(got - expected) <= tolerance;
            if (!ok && mismatches++ < 3) {
                first << "\n  cat-" << c << " t=" << t0 << " n=" << n << ": got " << got << ", expected " << expected;
            }
        };

        for (std::size_t t = 0; t < v.n_times; ++t) {
            for (std::size_t c : cats) {
                check(c, t, 1, v.missing.count({c, t}) ? std::numeric_limits<double>::quiet_NaN() : kelvin(c, t));
            }
        }
        // means over 6 timesteps, starting at several offsets, where nothing is missing
        for (std::size_t c : cats) {
            for (std::size_t t0 = 0; t0 + 6 <= v.n_times; t0 += 5) {
                double sum = 0;
                bool any_missing = false;
                for (std::size_t t = t0; t < t0 + 6; ++t) {
                    sum += kelvin(c, t);
                    any_missing |= v.missing.count({c, t}) > 0;
                }
                if (!any_missing) {
                    check(c, t0, 6, sum / 6);
                }
            }
        }
        EXPECT_EQ(mismatches, 0u) << "first mismatches:" << first.str();
    }

    // Writes the variant and checks reads of all catchments and of a
    // hinted subset that isn't contiguous in the file
    void expectEquivalent(const Variant& v) {
        auto path = write(v);
        expectReads(path, v);
        expectReads(path, v, {3, 0, 4});
    }

    std::vector<std::string> temp_files;

  private:
    // Attributes describing stored values must have the variable's type
    static void putTypedAtt(const netCDF::NcVar& var, const std::string& name, double value) {
        auto type = var.getType();
        if (type == netCDF::ncDouble) {
            var.putAtt(name, type, value);
        } else if (type == netCDF::ncFloat) {
            var.putAtt(name, type, float(value));
        } else if (type == netCDF::ncInt) {
            var.putAtt(name, type, int(value));
        } else if (type == netCDF::ncShort) {
            var.putAtt(name, type, short(value));
        } else {
            throw std::logic_error("unsupported type for " + name);
        }
    }
};

TEST_F(NetCDFStorageVariantTest, Canonical)
{
    expectEquivalent(Variant{});
}

TEST_F(NetCDFStorageVariantTest, CatchmentsInDifferentOrder)
{
    Variant v;
    v.order = {3, 0, 4, 1, 2};
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, ChunkedAndCompressed)
{
    Variant v;
    v.chunks = {2, 7};
    v.deflate_level = 5;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, ChunkedOneCatchmentPerChunk)
{
    Variant v;
    v.chunks = {1, v.n_times};
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, UnlimitedTimeDimension)
{
    Variant v;
    v.unlimited_time = true;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, FloatStorage)
{
    Variant v;
    v.type = Type::Float;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, ConvertibleUnits)
{
    Variant v;
    v.units = "degC";
    v.from_kelvin = [](double k) { return k - 273.15; };
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, TimeInMinutesWithEpochStart)
{
    Variant v;
    v.time_units = "minutes";
    v.epoch_start = "01/01/1970 00:00:00";
    v.encode_time = [](time_t e) { return e / 60.0; };
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, TimeInHoursSinceReferenceDate)
{
    Variant v;
    v.time_units = "hours since 2017-07-14 00:00:00";
    v.encode_time = [](time_t e) { return (e - 1499990400) / 3600.0; };
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, IntegerTimeValues)
{
    Variant v;
    v.time_type = Type::Int;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, MissingValuesAsNaN)
{
    Variant v;
    v.missing = {{1, 2}, {4, 19}};
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, ExtraVariablesAndDimensions)
{
    Variant v;
    v.extra_variables = true;
    expectEquivalent(v);
}

// Packed values are unpacked as stored * scale_factor + add_offset
TEST_F(NetCDFStorageVariantTest, PackedShortWithScaleAndOffset)
{
    Variant v;
    v.type = Type::Short;
    v.scale_factor = 0.01;
    v.add_offset = 250.0;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, PackedIntWithScaleOnly)
{
    Variant v;
    v.type = Type::Int;
    v.scale_factor = 0.001;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, PackedDoubleWithOffsetOnly)
{
    Variant v;
    v.add_offset = 250.0;
    expectEquivalent(v);
}

// Stored values marking missing data read as NaN, as values stored as NaN do
TEST_F(NetCDFStorageVariantTest, FillValueMarksMissing)
{
    Variant v;
    v.fill_value = -9999.0;
    v.missing = {{0, 0}, {2, 7}, {4, 19}};
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, PackedFillValueMarksMissing)
{
    Variant v;
    v.type = Type::Short;
    v.scale_factor = 0.01;
    v.add_offset = 250.0;
    v.fill_value = -32767;
    v.missing = {{0, 0}, {2, 7}, {4, 19}};
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, MissingValueAttributeMarksMissing)
{
    Variant v;
    v.missing_values = {1e20};
    v.missing = {{1, 3}, {3, 12}};
    expectEquivalent(v);
}

// Every value of a missing_value attribute marks missing data
TEST_F(NetCDFStorageVariantTest, MultipleMissingValuesMarkMissing)
{
    Variant v;
    v.missing_values = {1e20, -1e20};
    v.missing_raw = -1e20;
    v.missing = {{1, 3}, {3, 12}};
    expectEquivalent(v);
}

// Without a _FillValue attribute, the type's default fill value marks missing data
TEST_F(NetCDFStorageVariantTest, DefaultFillValueMarksMissing)
{
    Variant v;
    v.missing_raw = NC_FILL_DOUBLE;
    v.missing = {{1, 3}, {3, 12}};
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, FloatDefaultFillValueMarksMissing)
{
    Variant v;
    v.type = Type::Float;
    v.missing_raw = NC_FILL_FLOAT;
    v.missing = {{1, 3}, {3, 12}};
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, PackedDefaultFillValueMarksMissing)
{
    Variant v;
    v.type = Type::Int;
    v.scale_factor = 0.001;
    v.missing_raw = NC_FILL_INT;
    v.missing = {{1, 3}, {3, 12}};
    expectEquivalent(v);
}

// Variables can be stored with time as the leading dimension
TEST_F(NetCDFStorageVariantTest, TimeMajorDimensions)
{
    Variant v;
    v.time_major = true;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, TimeMajorDataWithCatchmentMajorTime)
{
    Variant v;
    v.time_major = true;
    v.time_var_time_major = false;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, CatchmentMajorDataWithTimeMajorTime)
{
    Variant v;
    v.time_var_time_major = true;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, TimeMajorCombined)
{
    Variant v;
    v.time_major = true;
    v.order = {2, 4, 0, 3, 1};
    v.unlimited_time = true;
    v.chunks = {4, 2};
    v.deflate_level = 2;
    v.type = Type::Short;
    v.scale_factor = 0.01;
    v.add_offset = 250.0;
    v.fill_value = -32767;
    v.missing = {{0, 5}, {4, 0}};
    v.extra_variables = true;
    expectEquivalent(v);
}

// Time units, and forms of CF reference dates, that UDUNITS and ISO 8601 allow
TEST_F(NetCDFStorageVariantTest, TimeInDaysSinceReferenceDate)
{
    Variant v;
    v.stride = 21600; // a quarter day, so stored times are exact
    v.time_units = "days since 2017-07-14 00:00:00";
    v.encode_time = [](time_t e) { return (e - 1499990400) / 86400.0; };
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, SingularAbbreviatedTimeUnit)
{
    Variant v;
    v.time_units = "hr since 2017-07-14 00:00:00";
    v.encode_time = [](time_t e) { return (e - 1499990400) / 3600.0; };
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, ReferenceDateWithoutTimeOfDay)
{
    Variant v;
    v.time_units = "hours since 2017-07-14";
    v.encode_time = [](time_t e) { return (e - 1499990400) / 3600.0; };
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, ReferenceDateWithoutSeconds)
{
    Variant v;
    v.time_units = "minutes since 2017-07-14 00:00";
    v.encode_time = [](time_t e) { return (e - 1499990400) / 60.0; };
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, ReferenceDateInIsoFormWithZ)
{
    Variant v;
    v.time_units = "hours since 2017-07-14T00:00:00Z";
    v.encode_time = [](time_t e) { return (e - 1499990400) / 3600.0; };
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, ReferenceDateWithFractionalSecondsAndUTC)
{
    Variant v;
    v.time_units = "seconds since 1970-01-01 00:00:00.0 UTC";
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, CombinedStorageVariations)
{
    Variant v;
    v.order = {2, 4, 0, 3, 1};
    v.unlimited_time = true;
    v.type = Type::Float;
    v.chunks = {3, 4};
    v.deflate_level = 2;
    v.units = "degC";
    v.from_kelvin = [](double k) { return k - 273.15; };
    v.extra_variables = true;
    expectEquivalent(v);
}

TEST_F(NetCDFStorageVariantTest, CombinedWithPacking)
{
    Variant v;
    v.order = {2, 4, 0, 3, 1};
    v.unlimited_time = true;
    v.type = Type::Short;
    v.scale_factor = 0.01;
    v.chunks = {2, 5};
    v.deflate_level = 2;
    v.units = "degC";
    v.from_kelvin = [](double k) { return k - 273.15; };
    expectEquivalent(v);
}
#endif
